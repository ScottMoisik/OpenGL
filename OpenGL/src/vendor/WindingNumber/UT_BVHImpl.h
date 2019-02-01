/*
 * Copyright (c) 2018 Side Effects Software Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * COMMENTS:
 *      Bounding Volume Hierarchy (BVH) implementation.
 *      The main file is UT_BVH.h; this file is separate so that
 *      files that don't actually need to call functions on the BVH
 *      won't have unnecessary headers and functions included.
 */

#pragma once

#ifndef __HDK_UT_BVHImpl_h__
#define __HDK_UT_BVHImpl_h__

#include "UT_BVH.h"
#include "UT_Array.h"
#include "UT_FixedVector.h"
//#include "UT_ParallelUtil.h"
#include "UT_SmallArray.h"
#include "SYS_Types.h"
#include <algorithm>
#include "blocked_range.h"
namespace HDK_Sample {

namespace UT {

template<typename T,uint NAXES>
SYS_FORCE_INLINE bool utBoxExclude(const UT::Box<T,NAXES>& box) noexcept {
    bool has_nan_or_inf = !SYSisFinite(box[0][0]);
    has_nan_or_inf |= !SYSisFinite(box[0][1]);
    for (uint axis = 1; axis < NAXES; ++axis)
    {
        has_nan_or_inf |= !SYSisFinite(box[axis][0]);
        has_nan_or_inf |= !SYSisFinite(box[axis][1]);
    }
    return has_nan_or_inf;
}
template<uint NAXES>
SYS_FORCE_INLINE bool utBoxExclude(const UT::Box<fpreal32,NAXES>& box) noexcept {
    const int32 *pboxints = reinterpret_cast<const int32*>(&box);
    // Fast check for NaN or infinity: check if exponent bits are 0xFF.
    bool has_nan_or_inf = ((pboxints[0] & 0x7F800000) == 0x7F800000);
    has_nan_or_inf |= ((pboxints[1] & 0x7F800000) == 0x7F800000);
    for (uint axis = 1; axis < NAXES; ++axis)
    {
        has_nan_or_inf |= ((pboxints[2*axis] & 0x7F800000) == 0x7F800000);
        has_nan_or_inf |= ((pboxints[2*axis + 1] & 0x7F800000) == 0x7F800000);
    }
    return has_nan_or_inf;
}
template<typename T,uint NAXES>
SYS_FORCE_INLINE T utBoxCenter(const UT::Box<T,NAXES>& box, uint axis) noexcept {
    const T* v = box.vals[axis];
    return v[0] + v[1];
}
template<typename T>
struct ut_BoxCentre {
    constexpr static uint scale = 2;
};
template<typename T,uint NAXES,bool INSTANTIATED>
SYS_FORCE_INLINE T utBoxExclude(const UT_FixedVector<T,NAXES,INSTANTIATED>& position) noexcept {
    bool has_nan_or_inf = !SYSisFinite(position[0]);
    for (uint axis = 1; axis < NAXES; ++axis)
        has_nan_or_inf |= !SYSisFinite(position[axis]);
    return has_nan_or_inf;
}
template<uint NAXES,bool INSTANTIATED>
SYS_FORCE_INLINE bool utBoxExclude(const UT_FixedVector<fpreal32,NAXES,INSTANTIATED>& position) noexcept {
    const int32 *ppositionints = reinterpret_cast<const int32*>(&position);
    // Fast check for NaN or infinity: check if exponent bits are 0xFF.
    bool has_nan_or_inf = ((ppositionints[0] & 0x7F800000) == 0x7F800000);
    for (uint axis = 1; axis < NAXES; ++axis)
        has_nan_or_inf |= ((ppositionints[axis] & 0x7F800000) == 0x7F800000);
    return has_nan_or_inf;
}
template<typename T,uint NAXES,bool INSTANTIATED>
SYS_FORCE_INLINE T utBoxCenter(const UT_FixedVector<T,NAXES,INSTANTIATED>& position, uint axis) noexcept {
    return position[axis];
}
template<typename T,uint NAXES,bool INSTANTIATED>
struct ut_BoxCentre<UT_FixedVector<T,NAXES,INSTANTIATED>> {
    constexpr static uint scale = 1;
};

template<typename BOX_TYPE,typename SRC_INT_TYPE,typename INT_TYPE>
INT_TYPE utExcludeNaNInfBoxIndices(const BOX_TYPE* boxes, SRC_INT_TYPE* indices, INT_TYPE& nboxes) noexcept 
{
    constexpr INT_TYPE PARALLEL_THRESHOLD = 65536;
    INT_TYPE ntasks = 1;
    if (nboxes >= PARALLEL_THRESHOLD) 
    {
        INT_TYPE nprocessors = UT_Thread::getNumProcessors();
        ntasks = (nprocessors > 1) ? SYSmin(4*nprocessors, nboxes/(PARALLEL_THRESHOLD/2)) : 1;
    }
    if (ntasks == 1) 
    {
        // Serial: easy case; just loop through.

        const SRC_INT_TYPE* indices_end = indices + nboxes;

        // Loop through forward once
        SRC_INT_TYPE* psrc_index = indices;
        for (; psrc_index != indices_end; ++psrc_index) 
	{
            const bool exclude = utBoxExclude(boxes[*psrc_index]);
            if (exclude)
                break;
        }
        if (psrc_index == indices_end)
            return 0;

        // First NaN or infinite box
        SRC_INT_TYPE* nan_start = psrc_index;
        for (++psrc_index; psrc_index != indices_end; ++psrc_index) 
	{
            const bool exclude = utBoxExclude(boxes[*psrc_index]);
            if (!exclude) 
	    {
                *nan_start = *psrc_index;
                ++nan_start;
            }
        }
        nboxes = nan_start-indices;
        return indices_end - nan_start;
    }

    // Parallel: hard case.
    // 1) Collapse each of ntasks chunks and count number of items to exclude
    // 2) Accumulate number of items to exclude.
    // 3) If none, return.
    // 4) Copy non-NaN chunks

    UT_SmallArray<INT_TYPE> nexcluded;
    nexcluded.setSizeNoInit(ntasks);
	tbb::blocked_range<INT_TYPE>& r;
    //UTparallelFor(UT_BlockedRange<INT_TYPE>(0,ntasks), [boxes,indices,ntasks,nboxes,&nexcluded]() 
    //{
        for (INT_TYPE taski = r.begin(), task_end = r.end(); taski < task_end; ++taski)
        {
            SRC_INT_TYPE* indices_start = indices + (taski*exint(nboxes))/ntasks;
            const SRC_INT_TYPE* indices_end = indices + ((taski+1)*exint(nboxes))/ntasks;
            SRC_INT_TYPE* psrc_index = indices_start;
            for (; psrc_index != indices_end; ++psrc_index)
            {
                const bool exclude = utBoxExclude(boxes[*psrc_index]);
                if (exclude)
                    break;
            }
            if (psrc_index == indices_end) 
	    {
                nexcluded[taski] = 0;
                continue;
            }

            // First NaN or infinite box
            SRC_INT_TYPE* nan_start = psrc_index;
            for (++psrc_index; psrc_index != indices_end; ++psrc_index) 
	    {
                const bool exclude = utBoxExclude(boxes[*psrc_index]);
                if (!exclude) 
		{
                    *nan_start = *psrc_index;
                    ++nan_start;
                }
            }
            nexcluded[taski] = indices_end - nan_start;
        }
    //}, 0, 1);

    // Accumulate
    INT_TYPE total_excluded = nexcluded[0];
    for (INT_TYPE taski = 1; taski < ntasks; ++taski) 
    {
        total_excluded += nexcluded[taski];
    }

    if (total_excluded == 0)
        return 0;

    // TODO: Parallelize this part, if it's a bottleneck and we care about cases with NaNs or infinities.

    INT_TYPE taski = 0;
    while (nexcluded[taski] == 0) 
    {
        ++taski;
    }

    SRC_INT_TYPE* dest_indices = indices + ((taski+1)*exint(nboxes))/ntasks - nexcluded[taski];

    SRC_INT_TYPE* dest_end = indices + nboxes - total_excluded;
    for (++taski; taski < ntasks && dest_indices < dest_end; ++taski)
    {
        const SRC_INT_TYPE* psrc_index = indices + (taski*exint(nboxes))/ntasks;
        const SRC_INT_TYPE* psrc_end = indices + ((taski+1)*exint(nboxes))/ntasks - nexcluded[taski];
        INT_TYPE count = psrc_end - psrc_index;
	// Note should be memmove as it is overlapping.
	memmove(dest_indices, psrc_index, sizeof(SRC_INT_TYPE)*count);
        dest_indices += count;
    }
    nboxes -= total_excluded;
    return total_excluded;
}

template<uint N>
template<BVH_Heuristic H,typename T,uint NAXES,typename BOX_TYPE,typename SRC_INT_TYPE>
void BVH<N>::init(const BOX_TYPE* boxes, const INT_TYPE nboxes, SRC_INT_TYPE* indices, bool reorder_indices, INT_TYPE max_items_per_leaf) noexcept {
    Box<T,NAXES> axes_minmax;
    computeFullBoundingBox(axes_minmax, boxes, nboxes, indices);

    init<H>(axes_minmax, boxes, nboxes, indices, reorder_indices, max_items_per_leaf);
}

template<uint N>
template<BVH_Heuristic H,typename T,uint NAXES,typename BOX_TYPE,typename SRC_INT_TYPE>
void BVH<N>::init(Box<T,NAXES> axes_minmax, const BOX_TYPE* boxes, INT_TYPE nboxes, SRC_INT_TYPE* indices, bool reorder_indices, INT_TYPE max_items_per_leaf) noexcept {
    // Clear the tree in advance to save memory.
    myRoot.reset();

    if (nboxes == 0) {
        myNumNodes = 0;
        return;
    }

    UT_Array<INT_TYPE> local_indices;
    if (!indices) {
        local_indices.setSizeNoInit(nboxes);
        indices = local_indices.array();
        createTrivialIndices(indices, nboxes);
    }

    // Exclude any boxes with NaNs or infinities by shifting down indices
    // over the bad box indices and updating nboxes.
    INT_TYPE nexcluded = utExcludeNaNInfBoxIndices(boxes, indices, nboxes);
    if (nexcluded != 0) {
        if (nboxes == 0) {
            myNumNodes = 0;
            return;
        }
        computeFullBoundingBox(axes_minmax, boxes, nboxes, indices);
    }

    UT_Array<Node> nodes;
    // Preallocate an overestimate of the number of nodes needed.
    nodes.setCapacity(nodeEstimate(nboxes));
    nodes.setSize(1);
    if (reorder_indices)
        initNodeReorder<H>(nodes, nodes[0], axes_minmax, boxes, indices, nboxes, 0, max_items_per_leaf);
    else
        initNode<H>(nodes, nodes[0], axes_minmax, boxes, indices, nboxes);

    // If capacity is more than 12.5% over the size, rellocate.
    if (8*nodes.capacity() > 9*nodes.size()) {
        nodes.setCapacity(nodes.size());
    }
    // Steal ownership of the array from the UT_Array
    myRoot.reset(nodes.array());
    myNumNodes = nodes.size();
    nodes.unsafeClearData();
}

template<uint N>
template<typename LOCAL_DATA,typename FUNCTORS>
void BVH<N>::traverse(
    FUNCTORS &functors,
    LOCAL_DATA* data_for_parent) const noexcept
{
    if (!myRoot)
        return;

    // NOTE: The root is always index 0.
    traverseHelper(0, INT_TYPE(-1), functors, data_for_parent);
}
template<uint N>
template<typename LOCAL_DATA,typename FUNCTORS>
void BVH<N>::traverseHelper(
    INT_TYPE nodei,
    INT_TYPE parent_nodei,
    FUNCTORS &functors,
    LOCAL_DATA* data_for_parent) const noexcept
{
    const Node &node = myRoot[nodei];
    bool descend = functors.pre(nodei, data_for_parent);
    if (!descend)
        return;
    LOCAL_DATA local_data[N];
    INT_TYPE s;
    for (s = 0; s < N; ++s) {
        const INT_TYPE node_int = node.child[s];
        if (Node::isInternal(node_int)) {
            if (node_int == Node::EMPTY) {
                // NOTE: Anything after this will be empty too, so we can break.
                break;
            }
            traverseHelper(Node::getInternalNum(node_int), nodei, functors, &local_data[s]);
        }
        else {
            functors.item(node_int, nodei, local_data[s]);
        }
    }
    // NOTE: s is now the number of non-empty entries in this node.
    functors.post(nodei, parent_nodei, data_for_parent, s, local_data);
}

template<uint N>
template<typename LOCAL_DATA,typename FUNCTORS>
void BVH<N>::traverseParallel(
    INT_TYPE parallel_threshold,
    FUNCTORS& functors,
    LOCAL_DATA* data_for_parent) const noexcept
{
    if (!myRoot)
        return;

    // NOTE: The root is always index 0.
    traverseParallelHelper(0, INT_TYPE(-1), parallel_threshold, myNumNodes, functors, data_for_parent);
}
template<uint N>
template<typename LOCAL_DATA,typename FUNCTORS>
void BVH<N>::traverseParallelHelper(
    INT_TYPE nodei,
    INT_TYPE parent_nodei,
    INT_TYPE parallel_threshold,
    INT_TYPE next_node_id,
    FUNCTORS& functors,
    LOCAL_DATA* data_for_parent) const noexcept
{
    const Node &node = myRoot[nodei];
    bool descend = functors.pre(nodei, data_for_parent);
    if (!descend)
        return;

    // To determine the number of nodes in a child's subtree, we take the next
    // node ID minus the current child's node ID.
    INT_TYPE next_nodes[N];
    INT_TYPE nnodes[N];
    INT_TYPE nchildren = N;
    INT_TYPE nparallel = 0;
    // s is currently unsigned, so we check s < N for bounds check.
    // The s >= 0 check is in case s ever becomes signed, and should be
    // automatically removed by the compiler for unsigned s.
    for (INT_TYPE s = N-1; (std::is_signed<INT_TYPE>::value ? (s >= 0) : (s < N)); --s) {
        const INT_TYPE node_int = node.child[s];
        if (node_int == Node::EMPTY) {
            --nchildren;
            continue;
        }
        next_nodes[s] = next_node_id;
        if (Node::isInternal(node_int)) {
            // NOTE: This depends on BVH<N>::initNode appending the child nodes
            //       in between their content, instead of all at once.
            INT_TYPE child_node_id = Node::getInternalNum(node_int);
            nnodes[s] = next_node_id - child_node_id;
            next_node_id = child_node_id;
        }
        else {
            nnodes[s] = 0;
        }
        nparallel += (nnodes[s] >= parallel_threshold);
    }

    LOCAL_DATA local_data[N];
    if (nparallel >= 2) {
        // Do any non-parallel ones first
        if (nparallel < nchildren) {
            for (INT_TYPE s = 0; s < N; ++s) {
                if (nnodes[s] >= parallel_threshold) {
                    continue;
                }
                const INT_TYPE node_int = node.child[s];
                if (Node::isInternal(node_int)) {
                    if (node_int == Node::EMPTY) {
                        // NOTE: Anything after this will be empty too, so we can break.
                        break;
                    }
                    traverseHelper(Node::getInternalNum(node_int), nodei, functors, &local_data[s]);
                }
                else {
                    functors.item(node_int, nodei, local_data[s]);
                }
            }
        }
        // Now do the parallel ones
		tbb::blocked_range<INT_TYPE>& r;
        //UTparallelFor(UT_BlockedRange<INT_TYPE>(0,nparallel), [this,nodei,&node,&nnodes,&next_nodes,&parallel_threshold,&functors,&local_data](const UT_BlockedRange<INT_TYPE>& r) {
            for (INT_TYPE taski = r.begin(); taski < r.end(); ++taski) {
                INT_TYPE parallel_count = 0;
                // NOTE: The check for s < N is just so that the compiler can
                //       (hopefully) figure out that it can fully unroll the loop.
                INT_TYPE s;
                for (s = 0; s < N; ++s) {
                    if (nnodes[s] < parallel_threshold) {
                        continue;
                    }
                    if (parallel_count == taski) {
                        break;
                    }
                    ++parallel_count;
                }
                const INT_TYPE node_int = node.child[s];
                if (Node::isInternal(node_int)) {
                    UT_ASSERT_MSG_P(node_int != Node::EMPTY, "Empty entries should have been excluded above.");
                    traverseParallelHelper(Node::getInternalNum(node_int), nodei, parallel_threshold, next_nodes[s], functors, &local_data[s]);
                }
                else {
                    functors.item(node_int, nodei, local_data[s]);
                }
            }
        //}, 0, 1);
    }
    else {
        // All in serial
        for (INT_TYPE s = 0; s < N; ++s) {
            const INT_TYPE node_int = node.child[s];
            if (Node::isInternal(node_int)) {
                if (node_int == Node::EMPTY) {
                    // NOTE: Anything after this will be empty too, so we can break.
                    break;
                }
                traverseHelper(Node::getInternalNum(node_int), nodei, functors, &local_data[s]);
            }
            else {
                functors.item(node_int, nodei, local_data[s]);
            }
        }
    }
    functors.post(nodei, parent_nodei, data_for_parent, nchildren, local_data);
}

template<uint N>
template<typename LOCAL_DATA,typename FUNCTORS>
void BVH<N>::traverseVector(
    FUNCTORS &functors,
    LOCAL_DATA* data_for_parent) const noexcept
{
    if (!myRoot)
        return;

    // NOTE: The root is always index 0.
    traverseVectorHelper(0, INT_TYPE(-1), functors, data_for_parent);
}
template<uint N>
template<typename LOCAL_DATA,typename FUNCTORS>
void BVH<N>::traverseVectorHelper(
    INT_TYPE nodei,
    INT_TYPE parent_nodei,
    FUNCTORS &functors,
    LOCAL_DATA* data_for_parent) const noexcept
{
    const Node &node = myRoot[nodei];
    INT_TYPE descend = functors.pre(nodei, data_for_parent);
    if (!descend)
        return;
    LOCAL_DATA local_data[N];
    INT_TYPE s;
    for (s = 0; s < N; ++s) {
        if ((descend>>s) & 1) {
            const INT_TYPE node_int = node.child[s];
            if (Node::isInternal(node_int)) {
                if (node_int == Node::EMPTY) {
                    // NOTE: Anything after this will be empty too, so we can break.
                    descend &= (INT_TYPE(1)<<s)-1;
                    break;
                }
                traverseVectorHelper(Node::getInternalNum(node_int), nodei, functors, &local_data[s]);
            }
            else {
                functors.item(node_int, nodei, local_data[s]);
            }
        }
    }
    // NOTE: s is now the number of non-empty entries in this node.
    functors.post(nodei, parent_nodei, data_for_parent, s, local_data, descend);
}

template<uint N>
template<typename SRC_INT_TYPE>
void BVH<N>::createTrivialIndices(SRC_INT_TYPE* indices, const INT_TYPE n) noexcept {
    constexpr INT_TYPE PARALLEL_THRESHOLD = 65536;
    INT_TYPE ntasks = 1;
    if (n >= PARALLEL_THRESHOLD) {
        INT_TYPE nprocessors = UT_Thread::getNumProcessors();
        ntasks = (nprocessors > 1) ? SYSmin(4*nprocessors, n/(PARALLEL_THRESHOLD/2)) : 1;
    }
    if (ntasks == 1) {
        for (INT_TYPE i = 0; i < n; ++i) {
            indices[i] = i;
        }
    }
    else {
		tbb::blocked_range<INT_TYPE>& r;
        //UTparallelFor(UT_BlockedRange<INT_TYPE>(0,ntasks), [indices,ntasks,n](const UT_BlockedRange<INT_TYPE>& r) {
            for (INT_TYPE taski = r.begin(), taskend = r.end(); taski != taskend; ++taski) {
                INT_TYPE start = (taski * exint(n))/ntasks;
                INT_TYPE end = ((taski+1) * exint(n))/ntasks;
                for (INT_TYPE i = start; i != end; ++i) {
                    indices[i] = i;
                }
            }
        //}, 0, 1);
    }
}

template<uint N>
template<typename T,uint NAXES,typename BOX_TYPE,typename SRC_INT_TYPE>
void BVH<N>::computeFullBoundingBox(Box<T,NAXES>& axes_minmax, const BOX_TYPE* boxes, const INT_TYPE nboxes, SRC_INT_TYPE* indices) noexcept {
    if (!nboxes) {
        axes_minmax.initBounds();
        return;
    }
    INT_TYPE ntasks = 1;
    if (nboxes >= 2*4096) {
		INT_TYPE nprocessors = 1;// UT_Thread::getNumProcessors();
        ntasks = (nprocessors > 1) ? SYSmin(4*nprocessors, nboxes/4096) : 1;
    }
    if (ntasks == 1) {
        Box<T,NAXES> box;
        if (indices) {
            box.initBounds(boxes[indices[0]]);
            for (INT_TYPE i = 1; i < nboxes; ++i) {
                box.combine(boxes[indices[i]]);
            }
        }
        else {
            box.initBounds(boxes[0]);
            for (INT_TYPE i = 1; i < nboxes; ++i) {
                box.combine(boxes[i]);
            }
        }
        axes_minmax = box;
    }
    else {
        // Combine boxes in parallel, into just a few boxes
        UT_SmallArray<Box<T,NAXES>> parallel_boxes;
        parallel_boxes.setSize(ntasks);
		tbb::blocked_range<INT_TYPE> r;
        //UTparallelFor(UT_BlockedRange<INT_TYPE>(0,ntasks), [&parallel_boxes,ntasks,boxes,nboxes,indices](const UT_BlockedRange<INT_TYPE>& r) {
            for (INT_TYPE taski = r.begin(), end = r.end(); taski < end; ++taski) {
                const INT_TYPE startbox = (taski*uint64(nboxes))/ntasks;
                const INT_TYPE endbox = ((taski+1)*uint64(nboxes))/ntasks;
                Box<T,NAXES> box;
                if (indices) {
                    box.initBounds(boxes[indices[startbox]]);
                    for (INT_TYPE i = startbox+1; i < endbox; ++i) {
                        box.combine(boxes[indices[i]]);
                    }
                }
                else {
                    box.initBounds(boxes[startbox]);
                    for (INT_TYPE i = startbox+1; i < endbox; ++i) {
                        box.combine(boxes[i]);
                    }
                }
                parallel_boxes[taski] = box;
            }
        //}, 0, 1);

        // Combine parallel_boxes
        Box<T,NAXES> box = parallel_boxes[0];
        for (INT_TYPE i = 1; i < ntasks; ++i) {
            box.combine(parallel_boxes[i]);
        }

        axes_minmax = box;
    }
}

template<uint N>
template<BVH_Heuristic H,typename T,uint NAXES,typename BOX_TYPE,typename SRC_INT_TYPE>
void BVH<N>::initNode(UT_Array<Node>& nodes, Node &node, const Box<T,NAXES>& axes_minmax, const BOX_TYPE* boxes, SRC_INT_TYPE* indices, INT_TYPE nboxes) noexcept {
    if (nboxes <= N) {
        // Fits in one node
        for (INT_TYPE i = 0; i < nboxes; ++i) {
            node.child[i] = indices[i];
        }
        for (INT_TYPE i = nboxes; i < N; ++i) {
            node.child[i] = Node::EMPTY;
        }
        return;
    }

    SRC_INT_TYPE* sub_indices[N+1];
    Box<T,NAXES> sub_boxes[N];

    if (N == 2) {
        sub_indices[0] = indices;
        sub_indices[2] = indices+nboxes;
        split<H>(axes_minmax, boxes, indices, nboxes, sub_indices[1], &sub_boxes[0]);
    }
    else {
        multiSplit<H>(axes_minmax, boxes, indices, nboxes, sub_indices, sub_boxes);
    }

    // Count the number of nodes to run in parallel and fill in single items in this node
    INT_TYPE nparallel = 0;
    static constexpr INT_TYPE PARALLEL_THRESHOLD = 1024;
    for (INT_TYPE i = 0; i < N; ++i) {
        INT_TYPE sub_nboxes = sub_indices[i+1]-sub_indices[i];
        if (sub_nboxes == 1) {
            node.child[i] = sub_indices[i][0];
        }
        else if (sub_nboxes >= PARALLEL_THRESHOLD) {
            ++nparallel;
        }
    }

    // NOTE: Child nodes of this node need to be placed just before the nodes in
    //       their corresponding subtree, in between the subtrees, because
    //       traverseParallel uses the difference between the child node IDs
    //       to determine the number of nodes in the subtree.

    // Recurse
    if (nparallel >= 2) {
        // Do the parallel ones first, so that they can be inserted in the right place.
        // Although the choice may seem somewhat arbitrary, we need the results to be
        // identical whether we choose to parallelize or not, and in case we change the
        // threshold later.
        UT_SmallArray<UT_Array<Node>> parallel_nodes;
        parallel_nodes.setSize(nparallel);
        UT_SmallArray<Node> parallel_parent_nodes;
        parallel_parent_nodes.setSize(nparallel);
		/*
    Copyright (c) 2005-2018 Intel Corporation
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
        http://www.apache.org/licenses/LICENSE-2.0
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef __TBB_tbb_stddef_H
#define __TBB_tbb_stddef_H

// Marketing-driven product version
#define TBB_VERSION_MAJOR 2019
#define TBB_VERSION_MINOR 0

// Engineering-focused interface version
#define TBB_INTERFACE_VERSION 11002
#define TBB_INTERFACE_VERSION_MAJOR TBB_INTERFACE_VERSION/1000

// The oldest major interface version still supported
// To be used in SONAME, manifests, etc.
#define TBB_COMPATIBLE_INTERFACE_VERSION 2

#define __TBB_STRING_AUX(x) #x
#define __TBB_STRING(x) __TBB_STRING_AUX(x)

// We do not need defines below for resource processing on windows
#if !defined RC_INVOKED

// Define groups for Doxygen documentation
/**
 * @defgroup algorithms         Algorithms
 * @defgroup containers         Containers
 * @defgroup memory_allocation  Memory Allocation
 * @defgroup synchronization    Synchronization
 * @defgroup timing             Timing
 * @defgroup task_scheduling    Task Scheduling
 */

// Simple text that is displayed on the main page of Doxygen documentation.
/**
 * \mainpage Main Page
 *
 * Click the tabs above for information about the
 * - <a href="./modules.html">Modules</a> (groups of functionality) implemented by the library
 * - <a href="./annotated.html">Classes</a> provided by the library
 * - <a href="./files.html">Files</a> constituting the library.
 * .
 * Please note that significant part of TBB functionality is implemented in the form of
 * template functions, descriptions of which are not accessible on the <a href="./annotated.html">Classes</a>
 * tab. Use <a href="./modules.html">Modules</a> or <a href="./namespacemembers.html">Namespace/Namespace Members</a>
 * tabs to find them.
 *
 * Additional pieces of information can be found here
 * - \subpage concepts
 * .
 */

/** \page concepts TBB concepts
    A concept is a set of requirements to a type, which are necessary and sufficient
    for the type to model a particular behavior or a set of behaviors. Some concepts
    are specific to a particular algorithm (e.g. algorithm body), while other ones
    are common to several algorithms (e.g. range concept).
    All TBB algorithms make use of different classes implementing various concepts.
    Implementation classes are supplied by the user as type arguments of template
    parameters and/or as objects passed as function call arguments. The library
    provides predefined  implementations of some concepts (e.g. several kinds of
    \ref range_req "ranges"), while other ones must always be implemented by the user.
    TBB defines a set of minimal requirements each concept must conform to. Here is
    the list of different concepts hyperlinked to the corresponding requirements specifications:
    - \subpage range_req
    - \subpage parallel_do_body_req
    - \subpage parallel_for_body_req
    - \subpage parallel_reduce_body_req
    - \subpage parallel_scan_body_req
    - \subpage parallel_sort_iter_req
**/

// tbb_config.h should be included the first since it contains macro definitions used in other headers
#include "tbb_config.h"

#if _MSC_VER >=1400
    #define __TBB_EXPORTED_FUNC   __cdecl
    #define __TBB_EXPORTED_METHOD __thiscall
#else
    #define __TBB_EXPORTED_FUNC
    #define __TBB_EXPORTED_METHOD
#endif

#if __INTEL_COMPILER || _MSC_VER
#define __TBB_NOINLINE(decl) __declspec(noinline) decl
#elif __GNUC__
#define __TBB_NOINLINE(decl) decl __attribute__ ((noinline))
#else
#define __TBB_NOINLINE(decl) decl
#endif

#if __TBB_NOEXCEPT_PRESENT
#define __TBB_NOEXCEPT(expression) noexcept(expression)
#else
#define __TBB_NOEXCEPT(expression)
#endif

#include <cstddef>      /* Need size_t and ptrdiff_t */

#if _MSC_VER
    #define __TBB_tbb_windef_H
    //#include "internal/_tbb_windef.h"
    #undef __TBB_tbb_windef_H
#endif
#if !defined(_MSC_VER) || _MSC_VER>=1600
    #include <stdint.h>
#endif

//! Type for an assertion handler
typedef void(*assertion_handler_type)( const char* filename, int line, const char* expression, const char * comment );

#if __TBBMALLOC_BUILD
namespace rml { namespace internal {
 #define __TBB_ASSERT_RELEASE(predicate,message) ((predicate)?((void)0) : rml::internal::assertion_failure(__FILE__,__LINE__,#predicate,message))
#else
namespace tbb {
 #define __TBB_ASSERT_RELEASE(predicate,message) ((predicate)?((void)0) : tbb::assertion_failure(__FILE__,__LINE__,#predicate,message))
#endif

    //! Set assertion handler and return previous value of it.
    assertion_handler_type __TBB_EXPORTED_FUNC set_assertion_handler( assertion_handler_type new_handler );

    //! Process an assertion failure.
    /** Normally called from __TBB_ASSERT macro.
        If assertion handler is null, print message for assertion failure and abort.
        Otherwise call the assertion handler. */
    void __TBB_EXPORTED_FUNC assertion_failure( const char* filename, int line, const char* expression, const char* comment );

#if __TBBMALLOC_BUILD
}}  // namespace rml::internal
#else
} // namespace tbb
#endif

#if TBB_USE_ASSERT

    //! Assert that predicate is true.
    /** If predicate is false, print assertion failure message.
        If the comment argument is not NULL, it is printed as part of the failure message.
        The comment argument has no other effect. */
    #define __TBB_ASSERT(predicate,message) __TBB_ASSERT_RELEASE(predicate,message)

    #define __TBB_ASSERT_EX __TBB_ASSERT

#else /* !TBB_USE_ASSERT */

    //! No-op version of __TBB_ASSERT.
    #define __TBB_ASSERT(predicate,comment) ((void)0)
    //! "Extended" version is useful to suppress warnings if a variable is only used with an assert
    #define __TBB_ASSERT_EX(predicate,comment) ((void)(1 && (predicate)))

#endif /* !TBB_USE_ASSERT */

//! The namespace tbb contains all components of the library.
namespace tbb {

    namespace internal {
#if _MSC_VER && _MSC_VER<1600
        typedef __int8 int8_t;
        typedef __int16 int16_t;
        typedef __int32 int32_t;
        typedef __int64 int64_t;
        typedef unsigned __int8 uint8_t;
        typedef unsigned __int16 uint16_t;
        typedef unsigned __int32 uint32_t;
        typedef unsigned __int64 uint64_t;
#else /* Posix */
        using ::int8_t;
        using ::int16_t;
        using ::int32_t;
        using ::int64_t;
        using ::uint8_t;
        using ::uint16_t;
        using ::uint32_t;
        using ::uint64_t;
#endif /* Posix */
    } // namespace internal

    using std::size_t;
    using std::ptrdiff_t;

//! The function returns the interface version of the TBB shared library being used.
/**
 * The version it returns is determined at runtime, not at compile/link time.
 * So it can be different than the value of TBB_INTERFACE_VERSION obtained at compile time.
 */
extern "C" int __TBB_EXPORTED_FUNC TBB_runtime_interface_version();

/**
 * @cond INTERNAL
 * @brief Identifiers declared inside namespace internal should never be used directly by client code.
 */
namespace internal {

//! Compile-time constant that is upper bound on cache line/sector size.
/** It should be used only in situations where having a compile-time upper
    bound is more useful than a run-time exact answer.
    @ingroup memory_allocation */
const size_t NFS_MaxLineSize = 128;

/** Label for data that may be accessed from different threads, and that may eventually become wrapped
    in a formal atomic type.
    Note that no problems have yet been observed relating to the definition currently being empty,
    even if at least "volatile" would seem to be in order to avoid data sometimes temporarily hiding
    in a register (although "volatile" as a "poor man's atomic" lacks several other features of a proper
    atomic, some of which are now provided instead through specialized functions).
    Note that usage is intentionally compatible with a definition as qualifier "volatile",
    both as a way to have the compiler help enforce use of the label and to quickly rule out
    one potential issue.
    Note however that, with some architecture/compiler combinations, e.g. on IA-64 architecture, "volatile"
    also has non-portable memory semantics that are needlessly expensive for "relaxed" operations.
    Note that this must only be applied to data that will not change bit patterns when cast to/from
    an integral type of the same length; tbb::atomic must be used instead for, e.g., floating-point types.
    TODO: apply wherever relevant **/
#define __TBB_atomic // intentionally empty, see above

#if __TBB_OVERRIDE_PRESENT
#define __TBB_override override
#else
#define __TBB_override // formal comment only
#endif

#if __TBB_CPP17_FALLTHROUGH_PRESENT
#define __TBB_fallthrough [[fallthrough]]
#elif __TBB_FALLTHROUGH_PRESENT
#define __TBB_fallthrough __attribute__ ((fallthrough))
#else
#define __TBB_fallthrough
#endif

template<class T, size_t S, size_t R>
struct padded_base : T {
    char pad[S - R];
};
template<class T, size_t S> struct padded_base<T, S, 0> : T {};

//! Pads type T to fill out to a multiple of cache line size.
template<class T, size_t S = NFS_MaxLineSize>
struct padded : padded_base<T, S, sizeof(T) % S> {};

//! Extended variant of the standard offsetof macro
/** The standard offsetof macro is not sufficient for TBB as it can be used for
    POD-types only. The constant 0x1000 (not NULL) is necessary to appease GCC. **/
#define __TBB_offsetof(class_name, member_name) \
    ((ptrdiff_t)&(reinterpret_cast<class_name*>(0x1000)->member_name) - 0x1000)

//! Returns address of the object containing a member with the given name and address
#define __TBB_get_object_ref(class_name, member_name, member_addr) \
    (*reinterpret_cast<class_name*>((char*)member_addr - __TBB_offsetof(class_name, member_name)))

//! Throws std::runtime_error with what() returning error_code description prefixed with aux_info
void __TBB_EXPORTED_FUNC handle_perror( int error_code, const char* aux_info );

#if TBB_USE_EXCEPTIONS
    #define __TBB_TRY try
    #define __TBB_CATCH(e) catch(e)
    #define __TBB_THROW(e) throw e
    #define __TBB_RETHROW() throw
#else /* !TBB_USE_EXCEPTIONS */
    inline bool __TBB_false() { return false; }
    #define __TBB_TRY
    #define __TBB_CATCH(e) if ( tbb::internal::__TBB_false() )
    #define __TBB_THROW(e) tbb::internal::suppress_unused_warning(e)
    #define __TBB_RETHROW() ((void)0)
#endif /* !TBB_USE_EXCEPTIONS */

//! Report a runtime warning.
void __TBB_EXPORTED_FUNC runtime_warning( const char* format, ... );

#if TBB_USE_ASSERT
static void* const poisoned_ptr = reinterpret_cast<void*>(-1);

//! Set p to invalid pointer value.
//  Also works for regular (non-__TBB_atomic) pointers.
template<typename T>
inline void poison_pointer( T* __TBB_atomic & p ) { p = reinterpret_cast<T*>(poisoned_ptr); }

/** Expected to be used in assertions only, thus no empty form is defined. **/
template<typename T>
inline bool is_poisoned( T* p ) { return p == reinterpret_cast<T*>(poisoned_ptr); }
#else
template<typename T>
inline void poison_pointer( T* __TBB_atomic & ) {/*do nothing*/}
#endif /* !TBB_USE_ASSERT */

//! Cast between unrelated pointer types.
/** This method should be used sparingly as a last resort for dealing with
    situations that inherently break strict ISO C++ aliasing rules. */
// T is a pointer type because it will be explicitly provided by the programmer as a template argument;
// U is a referent type to enable the compiler to check that "ptr" is a pointer, deducing U in the process.
template<typename T, typename U>
inline T punned_cast( U* ptr ) {
    uintptr_t x = reinterpret_cast<uintptr_t>(ptr);
    return reinterpret_cast<T>(x);
}

//! Base class for types that should not be assigned.
class no_assign {
    // Deny assignment
    void operator=( const no_assign& );
public:
#if __GNUC__
    //! Explicitly define default construction, because otherwise gcc issues gratuitous warning.
    no_assign() {}
#endif /* __GNUC__ */
};

//! Base class for types that should not be copied or assigned.
class no_copy: no_assign {
    //! Deny copy construction
    no_copy( const no_copy& );
public:
    //! Allow default construction
    no_copy() {}
};

#if TBB_DEPRECATED_MUTEX_COPYING
class mutex_copy_deprecated_and_disabled {};
#else
// By default various implementations of mutexes are not copy constructible
// and not copy assignable.
class mutex_copy_deprecated_and_disabled : no_copy {};
#endif

//! A function to check if passed in pointer is aligned on a specific border
template<typename T>
inline bool is_aligned(T* pointer, uintptr_t alignment) {
    return 0==((uintptr_t)pointer & (alignment-1));
}

//! A function to check if passed integer is a power of 2
template<typename integer_type>
inline bool is_power_of_two(integer_type arg) {
    return arg && (0 == (arg & (arg - 1)));
}

//! A function to compute arg modulo divisor where divisor is a power of 2.
template<typename argument_integer_type, typename divisor_integer_type>
inline argument_integer_type modulo_power_of_two(argument_integer_type arg, divisor_integer_type divisor) {
    __TBB_ASSERT( is_power_of_two(divisor), "Divisor should be a power of two" );
    return (arg & (divisor - 1));
}


//! A function to determine if arg is a power of 2 at least as big as another power of 2.
// i.e. for strictly positive i and j, with j being a power of 2,
// determines whether i==j<<k for some nonnegative k (so i==j yields true).
template<typename argument_integer_type, typename power2_integer_type>
inline bool is_power_of_two_at_least(argument_integer_type arg, power2_integer_type power2) {
    __TBB_ASSERT( is_power_of_two(power2), "Divisor should be a power of two" );
    return 0 == (arg & (arg - power2));
}

//! Utility template function to prevent "unused" warnings by various compilers.
template<typename T1> void suppress_unused_warning( const T1& ) {}
template<typename T1, typename T2> void suppress_unused_warning( const T1&, const T2& ) {}
template<typename T1, typename T2, typename T3> void suppress_unused_warning( const T1&, const T2&, const T3& ) {}

// Struct to be used as a version tag for inline functions.
/** Version tag can be necessary to prevent loader on Linux from using the wrong
    symbol in debug builds (when inline functions are compiled as out-of-line). **/
struct version_tag_v3 {};

typedef version_tag_v3 version_tag;

} // internal

//! Dummy type that distinguishes splitting constructor from copy constructor.
/**
 * See description of parallel_for and parallel_reduce for example usages.
 * @ingroup algorithms
 */
class split {
};

//! Type enables transmission of splitting proportion from partitioners to range objects
/**
 * In order to make use of such facility Range objects must implement
 * splitting constructor with this type passed and initialize static
 * constant boolean field 'is_splittable_in_proportion' with the value
 * of 'true'
 */
class proportional_split: internal::no_assign {
public:
    proportional_split(size_t _left = 1, size_t _right = 1) : my_left(_left), my_right(_right) { }

    size_t left() const { return my_left; }
    size_t right() const { return my_right; }

    // used when range does not support proportional split
    operator split() const { return split(); }

#if __TBB_ENABLE_RANGE_FEEDBACK
    void set_proportion(size_t _left, size_t _right) {
        my_left = _left;
        my_right = _right;
    }
#endif
private:
    size_t my_left, my_right;
};

} // tbb

// Following is a set of classes and functions typically used in compile-time "metaprogramming".
// TODO: move all that to a separate header

#if __TBB_ALLOCATOR_TRAITS_PRESENT || __TBB_CPP11_SMART_POINTERS_PRESENT
#include <memory> // for allocator_traits, unique_ptr
#endif

#if __TBB_CPP11_RVALUE_REF_PRESENT || __TBB_CPP11_DECLTYPE_PRESENT || _LIBCPP_VERSION
#include <utility> // for std::move, std::forward, std::declval
#endif

namespace tbb {
namespace internal {

#if __TBB_CPP11_SMART_POINTERS_PRESENT && __TBB_CPP11_RVALUE_REF_PRESENT && __TBB_CPP11_VARIADIC_TEMPLATES_PRESENT
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args&&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
#endif

//! Class for determining type of std::allocator<T>::value_type.
template<typename T>
struct allocator_type {
    typedef T value_type;
};

#if _MSC_VER
//! Microsoft std::allocator has non-standard extension that strips const from a type.
template<typename T>
struct allocator_type<const T> {
    typedef T value_type;
};
#endif

// Ad-hoc implementation of true_type & false_type
// Intended strictly for internal use! For public APIs (traits etc), use C++11 analogues.
template <bool v>
struct bool_constant {
    static /*constexpr*/ const bool value = v;
};
typedef bool_constant<true> true_type;
typedef bool_constant<false> false_type;

#if __TBB_ALLOCATOR_TRAITS_PRESENT
using std::allocator_traits;
#else
template<typename allocator>
struct allocator_traits{
    typedef tbb::internal::false_type propagate_on_container_move_assignment;
};
#endif

//! A template to select either 32-bit or 64-bit constant as compile time, depending on machine word size.
template <unsigned u, unsigned long long ull >
struct select_size_t_constant {
    //Explicit cast is needed to avoid compiler warnings about possible truncation.
    //The value of the right size,   which is selected by ?:, is anyway not truncated or promoted.
    static const size_t value = (size_t)((sizeof(size_t)==sizeof(u)) ? u : ull);
};

#if __TBB_CPP11_RVALUE_REF_PRESENT
using std::move;
using std::forward;
#elif defined(_LIBCPP_NAMESPACE)
// libc++ defines "pre-C++11 move and forward" similarly to ours; use it to avoid name conflicts in some cases.
using std::_LIBCPP_NAMESPACE::move;
using std::_LIBCPP_NAMESPACE::forward;
#else
// It is assumed that cv qualifiers, if any, are part of the deduced type.
template <typename T>
T& move( T& x ) { return x; }
template <typename T>
T& forward( T& x ) { return x; }
#endif /* __TBB_CPP11_RVALUE_REF_PRESENT */

// Helper macros to simplify writing templates working with both C++03 and C++11.
#if __TBB_CPP11_RVALUE_REF_PRESENT
#define  __TBB_FORWARDING_REF(A) A&&
#else
// It is assumed that cv qualifiers, if any, are part of a deduced type.
// Thus this macro should not be used in public interfaces.
#define  __TBB_FORWARDING_REF(A) A&
#endif
#if __TBB_CPP11_VARIADIC_TEMPLATES_PRESENT
#define __TBB_PARAMETER_PACK ...
#define __TBB_PACK_EXPANSION(A) A...
#else
#define __TBB_PARAMETER_PACK
#define __TBB_PACK_EXPANSION(A) A
#endif /* __TBB_CPP11_VARIADIC_TEMPLATES_PRESENT */

#if __TBB_CPP11_DECLTYPE_PRESENT
#if __TBB_CPP11_DECLVAL_BROKEN
// Ad-hoc implementation of std::declval
template <class T> __TBB_FORWARDING_REF(T) declval() /*noexcept*/;
#else
using std::declval;
#endif
#endif

template <bool condition>
struct STATIC_ASSERTION_FAILED;

template <>
struct STATIC_ASSERTION_FAILED<false> { enum {value=1};};

template<>
struct STATIC_ASSERTION_FAILED<true>; //intentionally left undefined to cause compile time error

//! @endcond
}} // namespace tbb::internal

#if __TBB_STATIC_ASSERT_PRESENT
#define __TBB_STATIC_ASSERT(condition,msg) static_assert(condition,msg)
#else
//please note condition is intentionally inverted to get a bit more understandable error msg
#define __TBB_STATIC_ASSERT_IMPL1(condition,msg,line)       \
    enum {static_assert_on_line_##line = tbb::internal::STATIC_ASSERTION_FAILED<!(condition)>::value}

#define __TBB_STATIC_ASSERT_IMPL(condition,msg,line) __TBB_STATIC_ASSERT_IMPL1(condition,msg,line)
//! Verify condition, at compile time
#define __TBB_STATIC_ASSERT(condition,msg) __TBB_STATIC_ASSERT_IMPL(condition,msg,__LINE__)
#endif

#endif /* RC_INVOKED */
#endif /* __TBB_tbb_stddef_H */
tbb::blocked_range<INT_TYPE>& r;
        //UTparallelFor(UT_BlockedRange<INT_TYPE>(0,nparallel), [&parallel_nodes,&parallel_parent_nodes,&sub_indices,boxes,&sub_boxes](const UT_BlockedRange<INT_TYPE>& r) {
            for (INT_TYPE taski = r.begin(), end = r.end(); taski < end; ++taski) {
                // First, find which child this is
                INT_TYPE counted_parallel = 0;
                INT_TYPE sub_nboxes;
                INT_TYPE childi;
                for (childi = 0; childi < N; ++childi) {
                    sub_nboxes = sub_indices[childi+1]-sub_indices[childi];
                    if (sub_nboxes >= PARALLEL_THRESHOLD) {
                        if (counted_parallel == taski) {
                            break;
                        }
                        ++counted_parallel;
                    }
                }
                UT_ASSERT_P(counted_parallel == taski);

                UT_Array<Node>& local_nodes = parallel_nodes[taski];
                // Preallocate an overestimate of the number of nodes needed.
                // At worst, we could have only 2 children in every leaf, and
                // then above that, we have a geometric series with r=1/N and a=(sub_nboxes/2)/N
                // The true worst case might be a little worst than this, but
                // it's probably fairly unlikely.
                local_nodes.setCapacity(nodeEstimate(sub_nboxes));
                Node& parent_node = parallel_parent_nodes[taski];

                // We'll have to fix the internal node numbers in parent_node and local_nodes later
                initNode<H>(local_nodes, parent_node, sub_boxes[childi], boxes, sub_indices[childi], sub_nboxes);
            }
        //}, 0, 1);

        INT_TYPE counted_parallel = 0;
        for (INT_TYPE i = 0; i < N; ++i) {
            INT_TYPE sub_nboxes = sub_indices[i+1]-sub_indices[i];
            if (sub_nboxes != 1) {
                INT_TYPE local_nodes_start = nodes.size();
                node.child[i] = Node::markInternal(local_nodes_start);
                if (sub_nboxes >= PARALLEL_THRESHOLD) {
                    // First, adjust the root child node
                    Node child_node = parallel_parent_nodes[counted_parallel];
                    ++local_nodes_start;
                    for (INT_TYPE childi = 0; childi < N; ++childi) {
                        INT_TYPE child_child = child_node.child[childi];
                        if (Node::isInternal(child_child) && child_child != Node::EMPTY) {
                            child_child += local_nodes_start;
                            child_node.child[childi] = child_child;
                        }
                    }

                    // Make space in the array for the sub-child nodes
                    const UT_Array<Node>& local_nodes = parallel_nodes[counted_parallel];
                    ++counted_parallel;
                    INT_TYPE n = local_nodes.size();
                    nodes.bumpCapacity(local_nodes_start + n);
                    nodes.setSizeNoInit(local_nodes_start + n);
                    nodes[local_nodes_start-1] = child_node;
                }
                else {
                    nodes.bumpCapacity(local_nodes_start + 1);
                    nodes.setSizeNoInit(local_nodes_start + 1);
                    initNode<H>(nodes, nodes[local_nodes_start], sub_boxes[i], boxes, sub_indices[i], sub_nboxes);
                }
            }
        }

        // Now, adjust and copy all sub-child nodes that were made in parallel
        adjustParallelChildNodes<PARALLEL_THRESHOLD>(nparallel, nodes, node, parallel_nodes.array(), sub_indices);
    }
    else {
        for (INT_TYPE i = 0; i < N; ++i) {
            INT_TYPE sub_nboxes = sub_indices[i+1]-sub_indices[i];
            if (sub_nboxes != 1) {
                INT_TYPE local_nodes_start = nodes.size();
                node.child[i] = Node::markInternal(local_nodes_start);
                nodes.bumpCapacity(local_nodes_start + 1);
                nodes.setSizeNoInit(local_nodes_start + 1);
                initNode<H>(nodes, nodes[local_nodes_start], sub_boxes[i], boxes, sub_indices[i], sub_nboxes);
            }
        }
    }
}

template<uint N>
template<BVH_Heuristic H,typename T,uint NAXES,typename BOX_TYPE,typename SRC_INT_TYPE>
void BVH<N>::initNodeReorder(UT_Array<Node>& nodes, Node &node, const Box<T,NAXES>& axes_minmax, const BOX_TYPE* boxes, SRC_INT_TYPE* indices, INT_TYPE nboxes, const INT_TYPE indices_offset, const INT_TYPE max_items_per_leaf) noexcept {
    if (nboxes <= N) {
        // Fits in one node
        for (INT_TYPE i = 0; i < nboxes; ++i) {
            node.child[i] = indices_offset+i;
        }
        for (INT_TYPE i = nboxes; i < N; ++i) {
            node.child[i] = Node::EMPTY;
        }
        return;
    }

    SRC_INT_TYPE* sub_indices[N+1];
    Box<T,NAXES> sub_boxes[N];

    if (N == 2) {
        sub_indices[0] = indices;
        sub_indices[2] = indices+nboxes;
        split<H>(axes_minmax, boxes, indices, nboxes, sub_indices[1], &sub_boxes[0]);
    }
    else {
        multiSplit<H>(axes_minmax, boxes, indices, nboxes, sub_indices, sub_boxes);
    }

    // Move any children with max_items_per_leaf or fewer indices before any children with more,
    // for better cache coherence when we're accessing data in a corresponding array.
    INT_TYPE nleaves = 0;
    UT_SmallArray<SRC_INT_TYPE> leaf_indices;
    SRC_INT_TYPE leaf_sizes[N];
    INT_TYPE sub_nboxes0 = sub_indices[1]-sub_indices[0];
    if (sub_nboxes0 <= max_items_per_leaf) {
        leaf_sizes[0] = sub_nboxes0;
        for (int j = 0; j < sub_nboxes0; ++j)
            leaf_indices.append(sub_indices[0][j]);
        ++nleaves;
    }
    INT_TYPE sub_nboxes1 = sub_indices[2]-sub_indices[1];
    if (sub_nboxes1 <= max_items_per_leaf) {
        leaf_sizes[nleaves] = sub_nboxes1;
        for (int j = 0; j < sub_nboxes1; ++j)
            leaf_indices.append(sub_indices[1][j]);
        ++nleaves;
    }
    for (INT_TYPE i = 2; i < N; ++i) {
        INT_TYPE sub_nboxes = sub_indices[i+1]-sub_indices[i];
        if (sub_nboxes <= max_items_per_leaf) {
            leaf_sizes[nleaves] = sub_nboxes;
            for (int j = 0; j < sub_nboxes; ++j)
                leaf_indices.append(sub_indices[i][j]);
            ++nleaves;
        }
    }
    if (nleaves > 0) {
        // NOTE: i < N condition is because INT_TYPE is unsigned.
        //       i >= 0 condition is in case INT_TYPE is changed to signed.
        INT_TYPE move_distance = 0;
        INT_TYPE index_move_distance = 0;
        for (INT_TYPE i = N-1; (std::is_signed<INT_TYPE>::value ? (i >= 0) : (i < N)); --i) {
            INT_TYPE sub_nboxes = sub_indices[i+1]-sub_indices[i];
            if (sub_nboxes <= max_items_per_leaf) {
                ++move_distance;
                index_move_distance += sub_nboxes;
            }
            else if (move_distance > 0) {
                SRC_INT_TYPE *start_src_index = sub_indices[i];
                for (SRC_INT_TYPE *src_index = sub_indices[i+1]-1; src_index >= start_src_index; --src_index) {
                    src_index[index_move_distance] = src_index[0];
                }
                sub_indices[i+move_distance] = sub_indices[i]+index_move_distance;
            }
        }
        index_move_distance = 0;
        for (INT_TYPE i = 0; i < nleaves; ++i) {
            INT_TYPE sub_nboxes = leaf_sizes[i];
            sub_indices[i] = indices+index_move_distance;
            for (int j = 0; j < sub_nboxes; ++j)
                indices[index_move_distance+j] = leaf_indices[index_move_distance+j];
            index_move_distance += sub_nboxes;
        }
    }

    // Count the number of nodes to run in parallel and fill in single items in this node
    INT_TYPE nparallel = 0;
    static constexpr INT_TYPE PARALLEL_THRESHOLD = 1024;
    for (INT_TYPE i = 0; i < N; ++i) {
        INT_TYPE sub_nboxes = sub_indices[i+1]-sub_indices[i];
        if (sub_nboxes <= max_items_per_leaf) {
            node.child[i] = indices_offset+(sub_indices[i]-sub_indices[0]);
        }
        else if (sub_nboxes >= PARALLEL_THRESHOLD) {
            ++nparallel;
        }
    }

    // NOTE: Child nodes of this node need to be placed just before the nodes in
    //       their corresponding subtree, in between the subtrees, because
    //       traverseParallel uses the difference between the child node IDs
    //       to determine the number of nodes in the subtree.

    // Recurse
    if (nparallel >= 2) {
        // Do the parallel ones first, so that they can be inserted in the right place.
        // Although the choice may seem somewhat arbitrary, we need the results to be
        // identical whether we choose to parallelize or not, and in case we change the
        // threshold later.
        UT_SmallArray<UT_Array<Node>,4*sizeof(UT_Array<Node>)> parallel_nodes;
        parallel_nodes.setSize(nparallel);
        UT_SmallArray<Node,4*sizeof(Node)> parallel_parent_nodes;
        parallel_parent_nodes.setSize(nparallel);
		tbb::blocked_range<INT_TYPE>& r;
        //UTparallelFor(UT_BlockedRange<INT_TYPE>(0,nparallel), [&parallel_nodes,&parallel_parent_nodes,&sub_indices,boxes,&sub_boxes,indices_offset,max_items_per_leaf](const UT_BlockedRange<INT_TYPE>& r) {
            for (INT_TYPE taski = r.begin(), end = r.end(); taski < end; ++taski) {
                // First, find which child this is
                INT_TYPE counted_parallel = 0;
                INT_TYPE sub_nboxes;
                INT_TYPE childi;
                for (childi = 0; childi < N; ++childi) {
                    sub_nboxes = sub_indices[childi+1]-sub_indices[childi];
                    if (sub_nboxes >= PARALLEL_THRESHOLD) {
                        if (counted_parallel == taski) {
                            break;
                        }
                        ++counted_parallel;
                    }
                }
                UT_ASSERT_P(counted_parallel == taski);

                UT_Array<Node>& local_nodes = parallel_nodes[taski];
                // Preallocate an overestimate of the number of nodes needed.
                // At worst, we could have only 2 children in every leaf, and
                // then above that, we have a geometric series with r=1/N and a=(sub_nboxes/2)/N
                // The true worst case might be a little worst than this, but
                // it's probably fairly unlikely.
                local_nodes.setCapacity(nodeEstimate(sub_nboxes));
                Node& parent_node = parallel_parent_nodes[taski];

                // We'll have to fix the internal node numbers in parent_node and local_nodes later
                initNodeReorder<H>(local_nodes, parent_node, sub_boxes[childi], boxes, sub_indices[childi], sub_nboxes,
                    indices_offset+(sub_indices[childi]-sub_indices[0]), max_items_per_leaf);
            }
        //}, 0, 1);

        INT_TYPE counted_parallel = 0;
        for (INT_TYPE i = 0; i < N; ++i) {
            INT_TYPE sub_nboxes = sub_indices[i+1]-sub_indices[i];
            if (sub_nboxes > max_items_per_leaf) {
                INT_TYPE local_nodes_start = nodes.size();
                node.child[i] = Node::markInternal(local_nodes_start);
                if (sub_nboxes >= PARALLEL_THRESHOLD) {
                    // First, adjust the root child node
                    Node child_node = parallel_parent_nodes[counted_parallel];
                    ++local_nodes_start;
                    for (INT_TYPE childi = 0; childi < N; ++childi) {
                        INT_TYPE child_child = child_node.child[childi];
                        if (Node::isInternal(child_child) && child_child != Node::EMPTY) {
                            child_child += local_nodes_start;
                            child_node.child[childi] = child_child;
                        }
                    }

                    // Make space in the array for the sub-child nodes
                    const UT_Array<Node>& local_nodes = parallel_nodes[counted_parallel];
                    ++counted_parallel;
                    INT_TYPE n = local_nodes.size();
                    nodes.bumpCapacity(local_nodes_start + n);
                    nodes.setSizeNoInit(local_nodes_start + n);
                    nodes[local_nodes_start-1] = child_node;
                }
                else {
                    nodes.bumpCapacity(local_nodes_start + 1);
                    nodes.setSizeNoInit(local_nodes_start + 1);
                    initNodeReorder<H>(nodes, nodes[local_nodes_start], sub_boxes[i], boxes, sub_indices[i], sub_nboxes,
                        indices_offset+(sub_indices[i]-sub_indices[0]), max_items_per_leaf);
                }
            }
        }

        // Now, adjust and copy all sub-child nodes that were made in parallel
        adjustParallelChildNodes<PARALLEL_THRESHOLD>(nparallel, nodes, node, parallel_nodes.array(), sub_indices);
    }
    else {
        for (INT_TYPE i = 0; i < N; ++i) {
            INT_TYPE sub_nboxes = sub_indices[i+1]-sub_indices[i];
            if (sub_nboxes > max_items_per_leaf) {
                INT_TYPE local_nodes_start = nodes.size();
                node.child[i] = Node::markInternal(local_nodes_start);
                nodes.bumpCapacity(local_nodes_start + 1);
                nodes.setSizeNoInit(local_nodes_start + 1);
                initNodeReorder<H>(nodes, nodes[local_nodes_start], sub_boxes[i], boxes, sub_indices[i], sub_nboxes,
                    indices_offset+(sub_indices[i]-sub_indices[0]), max_items_per_leaf);
            }
        }
    }
}

template<uint N>
template<BVH_Heuristic H,typename T,uint NAXES,typename BOX_TYPE,typename SRC_INT_TYPE>
void BVH<N>::multiSplit(const Box<T,NAXES>& axes_minmax, const BOX_TYPE* boxes, SRC_INT_TYPE* indices, INT_TYPE nboxes, SRC_INT_TYPE* sub_indices[N+1], Box<T,NAXES> sub_boxes[N]) noexcept {
    sub_indices[0] = indices;
    sub_indices[2] = indices+nboxes;
    split<H>(axes_minmax, boxes, indices, nboxes, sub_indices[1], &sub_boxes[0]);

    if (N == 2) {
        return;
    }

    if (H == BVH_Heuristic::MEDIAN_MAX_AXIS) {
        SRC_INT_TYPE* sub_indices_startend[2*N];
        Box<T,NAXES> sub_boxes_unsorted[N];
        sub_boxes_unsorted[0] = sub_boxes[0];
        sub_boxes_unsorted[1] = sub_boxes[1];
        sub_indices_startend[0] = sub_indices[0];
        sub_indices_startend[1] = sub_indices[1];
        sub_indices_startend[2] = sub_indices[1];
        sub_indices_startend[3] = sub_indices[2];
        for (INT_TYPE nsub = 2; nsub < N; ++nsub) {
            SRC_INT_TYPE* selected_start = sub_indices_startend[0];
            SRC_INT_TYPE* selected_end = sub_indices_startend[1];
            Box<T,NAXES> sub_box = sub_boxes_unsorted[0];

            // Shift results back.
            for (INT_TYPE i = 0; i < nsub-1; ++i) {
                sub_indices_startend[2*i  ] = sub_indices_startend[2*i+2];
                sub_indices_startend[2*i+1] = sub_indices_startend[2*i+3];
            }
            for (INT_TYPE i = 0; i < nsub-1; ++i) {
                sub_boxes_unsorted[i] = sub_boxes_unsorted[i-1];
            }

            // Do the split
            split<H>(sub_box, boxes, selected_start, selected_end-selected_start, sub_indices_startend[2*nsub-1], &sub_boxes_unsorted[nsub]);
            sub_indices_startend[2*nsub-2] = selected_start;
            sub_indices_startend[2*nsub] = sub_indices_startend[2*nsub-1];
            sub_indices_startend[2*nsub+1] = selected_end;

            // Sort pointers so that they're in the correct order
            sub_indices[N] = indices+nboxes;
            for (INT_TYPE i = 0; i < N; ++i) {
                SRC_INT_TYPE* prev_pointer = (i != 0) ? sub_indices[i-1] : nullptr;
                SRC_INT_TYPE* min_pointer = nullptr;
                Box<T,NAXES> box;
                for (INT_TYPE j = 0; j < N; ++j) {
                    SRC_INT_TYPE* cur_pointer = sub_indices_startend[2*j];
                    if ((cur_pointer > prev_pointer) && (!min_pointer || (cur_pointer < min_pointer))) {
                        min_pointer = cur_pointer;
                        box = sub_boxes_unsorted[j];
                    }
                }
                UT_ASSERT_P(min_pointer);
                sub_indices[i] = min_pointer;
                sub_boxes[i] = box;
            }
        }
    }
    else {
        T sub_box_areas[N];
        sub_box_areas[0] = unweightedHeuristic<H>(sub_boxes[0]);
        sub_box_areas[1] = unweightedHeuristic<H>(sub_boxes[1]);
        for (INT_TYPE nsub = 2; nsub < N; ++nsub) {
            // Choose which one to split
            INT_TYPE split_choice = INT_TYPE(-1);
            T max_heuristic;
            for (INT_TYPE i = 0; i < nsub; ++i) {
                const INT_TYPE index_count = (sub_indices[i+1]-sub_indices[i]);
                if (index_count > 1) {
                    const T heuristic = sub_box_areas[i]*index_count;
                    if (split_choice == INT_TYPE(-1) || heuristic > max_heuristic) {
                        split_choice = i;
                        max_heuristic = heuristic;
                    }
                }
            }
            UT_ASSERT_MSG_P(split_choice != INT_TYPE(-1), "There should always be at least one that can be split!");

            SRC_INT_TYPE* selected_start = sub_indices[split_choice];
            SRC_INT_TYPE* selected_end = sub_indices[split_choice+1];

            // Shift results over; we can skip the one we selected.
            for (INT_TYPE i = nsub; i > split_choice; --i) {
                sub_indices[i+1] = sub_indices[i];
            }
            for (INT_TYPE i = nsub-1; i > split_choice; --i) {
                sub_boxes[i+1] = sub_boxes[i];
            }
            for (INT_TYPE i = nsub-1; i > split_choice; --i) {
                sub_box_areas[i+1] = sub_box_areas[i];
            }

            // Do the split
            split<H>(sub_boxes[split_choice], boxes, selected_start, selected_end-selected_start, sub_indices[split_choice+1], &sub_boxes[split_choice]);
            sub_box_areas[split_choice] = unweightedHeuristic<H>(sub_boxes[split_choice]);
            sub_box_areas[split_choice+1] = unweightedHeuristic<H>(sub_boxes[split_choice+1]);
        }
    }
}

template<uint N>
template<BVH_Heuristic H,typename T,uint NAXES,typename BOX_TYPE,typename SRC_INT_TYPE>
void BVH<N>::split(const Box<T,NAXES>& axes_minmax, const BOX_TYPE* boxes, SRC_INT_TYPE* indices, INT_TYPE nboxes, SRC_INT_TYPE*& split_indices, Box<T,NAXES>* split_boxes) noexcept {
    if (nboxes == 2) {
        split_boxes[0].initBounds(boxes[indices[0]]);
        split_boxes[1].initBounds(boxes[indices[1]]);
        split_indices = indices+1;
        return;
    }
    UT_ASSERT_MSG_P(nboxes > 2, "Cases with less than 3 boxes should have already been handled!");

    if (H == BVH_Heuristic::MEDIAN_MAX_AXIS) {
        UT_ASSERT_MSG(0, "FIXME: Implement this!!!");
    }

    constexpr INT_TYPE SMALL_LIMIT = 6;
    if (nboxes <= SMALL_LIMIT) {
        // Special case for a small number of boxes: check all (2^(n-1))-1 partitions.
        // Without loss of generality, we assume that box 0 is in partition 0,
        // and that not all boxes are in partition 0.
        Box<T,NAXES> local_boxes[SMALL_LIMIT];
        for (INT_TYPE box = 0; box < nboxes; ++box) {
            local_boxes[box].initBounds(boxes[indices[box]]);
            //printf("Box %u: (%f-%f)x(%f-%f)x(%f-%f)\n", uint(box), local_boxes[box].vals[0][0], local_boxes[box].vals[0][1], local_boxes[box].vals[1][0], local_boxes[box].vals[1][1], local_boxes[box].vals[2][0], local_boxes[box].vals[2][1]);
        }
        const INT_TYPE partition_limit = (INT_TYPE(1)<<(nboxes-1));
        INT_TYPE best_partition = INT_TYPE(-1);
        T best_heuristic;
        for (INT_TYPE partition_bits = 1; partition_bits < partition_limit; ++partition_bits) {
            Box<T,NAXES> sub_boxes[2];
            sub_boxes[0] = local_boxes[0];
            sub_boxes[1].initBounds();
            INT_TYPE sub_counts[2] = {1,0};
            for (INT_TYPE bit = 0; bit < nboxes-1; ++bit) {
                INT_TYPE dest = (partition_bits>>bit)&1;
                sub_boxes[dest].combine(local_boxes[bit+1]);
                ++sub_counts[dest];
            }
            //printf("Partition bits %u: sub_box[0]: (%f-%f)x(%f-%f)x(%f-%f)\n", uint(partition_bits), sub_boxes[0].vals[0][0], sub_boxes[0].vals[0][1], sub_boxes[0].vals[1][0], sub_boxes[0].vals[1][1], sub_boxes[0].vals[2][0], sub_boxes[0].vals[2][1]);
            //printf("Partition bits %u: sub_box[1]: (%f-%f)x(%f-%f)x(%f-%f)\n", uint(partition_bits), sub_boxes[1].vals[0][0], sub_boxes[1].vals[0][1], sub_boxes[1].vals[1][0], sub_boxes[1].vals[1][1], sub_boxes[1].vals[2][0], sub_boxes[1].vals[2][1]);
            const T heuristic =
                unweightedHeuristic<H>(sub_boxes[0])*sub_counts[0] +
                unweightedHeuristic<H>(sub_boxes[1])*sub_counts[1];
            //printf("Partition bits %u: heuristic = %f (= %f*%u + %f*%u)\n",uint(partition_bits),heuristic, unweightedHeuristic<H>(sub_boxes[0]), uint(sub_counts[0]), unweightedHeuristic<H>(sub_boxes[1]), uint(sub_counts[1]));
            if (best_partition == INT_TYPE(-1) || heuristic < best_heuristic) {
                //printf("    New best\n");
                best_partition = partition_bits;
                best_heuristic = heuristic;
                split_boxes[0] = sub_boxes[0];
                split_boxes[1] = sub_boxes[1];
            }
        }

#if 0 // This isn't actually necessary with the current design, because I changed how the number of subtree nodes is determined.
        // If best_partition is partition_limit-1, there's only 1 box
        // in partition 0.  We should instead put this in partition 1,
        // so that we can help always have the internal node indices first
        // in each node.  That gets used to (fairly) quickly determine
        // the number of nodes in a sub-tree.
        if (best_partition == partition_limit - 1) {
            // Put the first index last.
            SRC_INT_TYPE last_index = indices[0];
            SRC_INT_TYPE* dest_indices = indices;
            SRC_INT_TYPE* local_split_indices = indices + nboxes-1;
            for (; dest_indices != local_split_indices; ++dest_indices) {
                dest_indices[0] = dest_indices[1];
            }
            *local_split_indices = last_index;
            split_indices = local_split_indices;

            // Swap the boxes
            const Box<T,NAXES> temp_box = sub_boxes[0];
            sub_boxes[0] = sub_boxes[1];
            sub_boxes[1] = temp_box;
            return;
        }
#endif

        // Reorder the indices.
        // NOTE: Index 0 is always in partition 0, so can stay put.
        SRC_INT_TYPE local_indices[SMALL_LIMIT-1];
        for (INT_TYPE box = 0; box < nboxes-1; ++box) {
            local_indices[box] = indices[box+1];
        }
        SRC_INT_TYPE* dest_indices = indices+1;
        SRC_INT_TYPE* src_indices = local_indices;
        // Copy partition 0
        for (INT_TYPE bit = 0; bit < nboxes-1; ++bit, ++src_indices) {
            if (!((best_partition>>bit)&1)) {
                //printf("Copying %u into partition 0\n",uint(*src_indices));
                *dest_indices = *src_indices;
                ++dest_indices;
            }
        }
        split_indices = dest_indices;
        // Copy partition 1
        src_indices = local_indices;
        for (INT_TYPE bit = 0; bit < nboxes-1; ++bit, ++src_indices) {
            if ((best_partition>>bit)&1) {
                //printf("Copying %u into partition 1\n",uint(*src_indices));
                *dest_indices = *src_indices;
                ++dest_indices;
            }
        }
        return;
    }

    uint max_axis = 0;
    T max_axis_length = axes_minmax.vals[0][1] - axes_minmax.vals[0][0];
    for (uint axis = 1; axis < NAXES; ++axis) {
        const T axis_length = axes_minmax.vals[axis][1] - axes_minmax.vals[axis][0];
        if (axis_length > max_axis_length) {
            max_axis = axis;
            max_axis_length = axis_length;
        }
    }

    if (!(max_axis_length > T(0))) {
        // All boxes are a single point or NaN.
        // Pick an arbitrary split point.
        split_indices = indices + nboxes/2;
        split_boxes[0] = axes_minmax;
        split_boxes[1] = axes_minmax;
        return;
    }

    const INT_TYPE axis = max_axis;

    constexpr INT_TYPE MID_LIMIT = 2*NSPANS;
    if (nboxes <= MID_LIMIT) {
        // Sort along axis, and try all possible splits.

#if 1
        // First, compute midpoints
        T midpointsx2[MID_LIMIT];
        for (INT_TYPE i = 0; i < nboxes; ++i) {
            midpointsx2[i] = utBoxCenter(boxes[indices[i]], axis);
        }
        SRC_INT_TYPE local_indices[MID_LIMIT];
        for (INT_TYPE i = 0; i < nboxes; ++i) {
            local_indices[i] = i;
        }

        const INT_TYPE chunk_starts[5] = {0, nboxes/4, nboxes/2, INT_TYPE((3*uint64(nboxes))/4), nboxes};

        // For sorting, insertion sort 4 chunks and merge them
        for (INT_TYPE chunk = 0; chunk < 4; ++chunk) {
            const INT_TYPE start = chunk_starts[chunk];
            const INT_TYPE end = chunk_starts[chunk+1];
            for (INT_TYPE i = start+1; i < end; ++i) {
                SRC_INT_TYPE indexi = local_indices[i];
                T vi = midpointsx2[indexi];
                for (INT_TYPE j = start; j < i; ++j) {
                    SRC_INT_TYPE indexj = local_indices[j];
                    T vj = midpointsx2[indexj];
                    if (vi < vj) {
                        do {
                            local_indices[j] = indexi;
                            indexi = indexj;
                            ++j;
                            if (j == i) {
                                local_indices[j] = indexi;
                                break;
                            }
                            indexj = local_indices[j];
                        } while (true);
                        break;
                    }
                }
            }
        }
        // Merge chunks into another buffer
        SRC_INT_TYPE local_indices_temp[MID_LIMIT];
        std::merge(local_indices, local_indices+chunk_starts[1],
            local_indices+chunk_starts[1], local_indices+chunk_starts[2],
            local_indices_temp, [&midpointsx2](const SRC_INT_TYPE a, const SRC_INT_TYPE b)->bool {
            return midpointsx2[a] < midpointsx2[b];
        });
        std::merge(local_indices+chunk_starts[2], local_indices+chunk_starts[3],
            local_indices+chunk_starts[3], local_indices+chunk_starts[4],
            local_indices_temp+chunk_starts[2], [&midpointsx2](const SRC_INT_TYPE a, const SRC_INT_TYPE b)->bool {
            return midpointsx2[a] < midpointsx2[b];
        });
        std::merge(local_indices_temp, local_indices_temp+chunk_starts[2],
            local_indices_temp+chunk_starts[2], local_indices_temp+chunk_starts[4],
            local_indices, [&midpointsx2](const SRC_INT_TYPE a, const SRC_INT_TYPE b)->bool {
            return midpointsx2[a] < midpointsx2[b];
        });

        // Translate local_indices into indices
        for (INT_TYPE i = 0; i < nboxes; ++i) {
            local_indices[i] = indices[local_indices[i]];
        }
        // Copy back
        for (INT_TYPE i = 0; i < nboxes; ++i) {
            indices[i] = local_indices[i];
        }
#else
        std::stable_sort(indices, indices+nboxes, [boxes,max_axis](SRC_INT_TYPE a, SRC_INT_TYPE b)->bool {
            return utBoxCenter(boxes[a], max_axis) < utBoxCenter(boxes[b], max_axis);
        });
#endif

        // Accumulate boxes
        Box<T,NAXES> left_boxes[MID_LIMIT-1];
        Box<T,NAXES> right_boxes[MID_LIMIT-1];
        const INT_TYPE nsplits = nboxes-1;
        Box<T,NAXES> box_accumulator(boxes[local_indices[0]]);
        left_boxes[0] = box_accumulator;
        for (INT_TYPE i = 1; i < nsplits; ++i) {
            box_accumulator.combine(boxes[local_indices[i]]);
            left_boxes[i] = box_accumulator;
        }
        box_accumulator.initBounds(boxes[local_indices[nsplits-1]]);
        right_boxes[nsplits-1] = box_accumulator;
        for (INT_TYPE i = nsplits-1; i > 0; --i) {
            box_accumulator.combine(boxes[local_indices[i]]);
            right_boxes[i-1] = box_accumulator;
        }

        INT_TYPE best_split = 0;
        T best_local_heuristic =
            unweightedHeuristic<H>(left_boxes[0]) +
            unweightedHeuristic<H>(right_boxes[0])*(nboxes-1);
        for (INT_TYPE split = 1; split < nsplits; ++split) {
            const T heuristic =
                unweightedHeuristic<H>(left_boxes[split])*(split+1) +
                unweightedHeuristic<H>(right_boxes[split])*(nboxes-(split+1));
            if (heuristic < best_local_heuristic) {
                best_split = split;
                best_local_heuristic = heuristic;
            }
        }
        split_indices = indices+best_split+1;
        split_boxes[0] = left_boxes[best_split];
        split_boxes[1] = right_boxes[best_split];
        return;
    }

    const T axis_min = axes_minmax.vals[max_axis][0];
    const T axis_length = max_axis_length;
    Box<T,NAXES> span_boxes[NSPANS];
    for (INT_TYPE i = 0; i < NSPANS; ++i) {
        span_boxes[i].initBounds();
    }
    INT_TYPE span_counts[NSPANS];
    for (INT_TYPE i = 0; i < NSPANS; ++i) {
        span_counts[i] = 0;
    }

    const T axis_min_x2 = ut_BoxCentre<BOX_TYPE>::scale*axis_min;
    // NOTE: Factor of 0.5 is factored out of the average when using the average value to determine the span that a box lies in.
    const T axis_index_scale = (T(1.0/ut_BoxCentre<BOX_TYPE>::scale)*NSPANS)/axis_length;
    constexpr INT_TYPE BOX_SPANS_PARALLEL_THRESHOLD = 2048;
    INT_TYPE ntasks = 1;
    if (nboxes >= BOX_SPANS_PARALLEL_THRESHOLD) {
        INT_TYPE nprocessors = UT_Thread::getNumProcessors();
        ntasks = (nprocessors > 1) ? SYSmin(4*nprocessors, nboxes/(BOX_SPANS_PARALLEL_THRESHOLD/2)) : 1;
    }
    if (ntasks == 1) {
        for (INT_TYPE indexi = 0; indexi < nboxes; ++indexi) {
            const auto& box = boxes[indices[indexi]];
            const T sum = utBoxCenter(box, axis);
            const uint span_index = SYSclamp(int((sum-axis_min_x2)*axis_index_scale), int(0), int(NSPANS-1));
            ++span_counts[span_index];
            Box<T,NAXES>& span_box = span_boxes[span_index];
            span_box.combine(box);
        }
    }
    else {
        // Combine boxes in parallel, into just a few boxes
        UT_SmallArray<Box<T,NAXES>> parallel_boxes;
        parallel_boxes.setSize(NSPANS*ntasks);
        UT_SmallArray<INT_TYPE> parallel_counts;
        parallel_counts.setSize(NSPANS*ntasks);
		tbb::blocked_range<INT_TYPE>& r;
        //UTparallelFor(UT_BlockedRange<INT_TYPE>(0,ntasks), [&parallel_boxes,&parallel_counts,ntasks,boxes,nboxes,indices,axis,axis_min_x2,axis_index_scale](const UT_BlockedRange<INT_TYPE>& r) {
            for (INT_TYPE taski = r.begin(), end = r.end(); taski < end; ++taski) {
                Box<T,NAXES> span_boxes[NSPANS];
                for (INT_TYPE i = 0; i < NSPANS; ++i) {
                    span_boxes[i].initBounds();
                }
                INT_TYPE span_counts[NSPANS];
                for (INT_TYPE i = 0; i < NSPANS; ++i) {
                    span_counts[i] = 0;
                }
                const INT_TYPE startbox = (taski*uint64(nboxes))/ntasks;
                const INT_TYPE endbox = ((taski+1)*uint64(nboxes))/ntasks;
                for (INT_TYPE indexi = startbox; indexi != endbox; ++indexi) {
                    const auto& box = boxes[indices[indexi]];
                    const T sum = utBoxCenter(box, axis);
                    const uint span_index = SYSclamp(int((sum-axis_min_x2)*axis_index_scale), int(0), int(NSPANS-1));
                    ++span_counts[span_index];
                    Box<T,NAXES>& span_box = span_boxes[span_index];
                    span_box.combine(box);
                }
                // Copy the results out
                const INT_TYPE dest_array_start = taski*NSPANS;
                for (INT_TYPE i = 0; i < NSPANS; ++i) {
                    parallel_boxes[dest_array_start+i] = span_boxes[i];
                }
                for (INT_TYPE i = 0; i < NSPANS; ++i) {
                    parallel_counts[dest_array_start+i] = span_counts[i];
                }
            }
        //}, 0, 1);

        // Combine the partial results
        for (INT_TYPE taski = 0; taski < ntasks; ++taski) {
            const INT_TYPE dest_array_start = taski*NSPANS;
            for (INT_TYPE i = 0; i < NSPANS; ++i) {
                span_boxes[i].combine(parallel_boxes[dest_array_start+i]);
            }
        }
        for (INT_TYPE taski = 0; taski < ntasks; ++taski) {
            const INT_TYPE dest_array_start = taski*NSPANS;
            for (INT_TYPE i = 0; i < NSPANS; ++i) {
                span_counts[i] += parallel_counts[dest_array_start+i];
            }
        }
    }

    // Spans 0 to NSPANS-2
    Box<T,NAXES> left_boxes[NSPLITS];
    // Spans 1 to NSPANS-1
    Box<T,NAXES> right_boxes[NSPLITS];

    // Accumulate boxes
    Box<T,NAXES> box_accumulator = span_boxes[0];
    left_boxes[0] = box_accumulator;
    for (INT_TYPE i = 1; i < NSPLITS; ++i) {
        box_accumulator.combine(span_boxes[i]);
        left_boxes[i] = box_accumulator;
    }
    box_accumulator = span_boxes[NSPANS-1];
    right_boxes[NSPLITS-1] = box_accumulator;
    for (INT_TYPE i = NSPLITS-1; i > 0; --i) {
        box_accumulator.combine(span_boxes[i]);
        right_boxes[i-1] = box_accumulator;
    }

    INT_TYPE left_counts[NSPLITS];

    // Accumulate counts
    INT_TYPE count_accumulator = span_counts[0];
    left_counts[0] = count_accumulator;
    for (INT_TYPE spliti = 1; spliti < NSPLITS; ++spliti) {
        count_accumulator += span_counts[spliti];
        left_counts[spliti] = count_accumulator;
    }

    // Check which split is optimal, making sure that at least 1/MIN_FRACTION of all boxes are on each side.
    const INT_TYPE min_count = nboxes/MIN_FRACTION;
    UT_ASSERT_MSG_P(min_count > 0, "MID_LIMIT above should have been large enough that nboxes would be > MIN_FRACTION");
    const INT_TYPE max_count = ((MIN_FRACTION-1)*uint64(nboxes))/MIN_FRACTION;
    UT_ASSERT_MSG_P(max_count < nboxes, "I'm not sure how this could happen mathematically, but it needs to be checked.");
    T smallest_heuristic = std::numeric_limits<T>::infinity();
    INT_TYPE split_index = -1;
    for (INT_TYPE spliti = 0; spliti < NSPLITS; ++spliti) {
        const INT_TYPE left_count = left_counts[spliti];
        if (left_count < min_count || left_count > max_count) {
            continue;
        }
        const INT_TYPE right_count = nboxes-left_count;
        const T heuristic =
            left_count*unweightedHeuristic<H>(left_boxes[spliti]) +
            right_count*unweightedHeuristic<H>(right_boxes[spliti]);
        if (heuristic < smallest_heuristic) {
            smallest_heuristic = heuristic;
            split_index = spliti;
        }
    }

    SRC_INT_TYPE*const indices_end = indices+nboxes;

    if (split_index == -1) {
        // No split was anywhere close to balanced, so we fall back to searching for one.

        // First, find the span containing the "balance" point, namely where left_counts goes from
        // being less than min_count to more than max_count.
        // If that's span 0, use max_count as the ordered index to select,
        // if it's span NSPANS-1, use min_count as the ordered index to select,
        // else use nboxes/2 as the ordered index to select.
        //T min_pivotx2 = -std::numeric_limits<T>::infinity();
        //T max_pivotx2 = std::numeric_limits<T>::infinity();
        SRC_INT_TYPE* nth_index;
        if (left_counts[0] > max_count) {
            // Search for max_count ordered index
            nth_index = indices+max_count;
            //max_pivotx2 = max_axis_min_x2 + max_axis_length/(NSPANS/ut_BoxCentre<BOX_TYPE>::scale);
        }
        else if (left_counts[NSPLITS-1] < min_count) {
            // Search for min_count ordered index
            nth_index = indices+min_count;
            //min_pivotx2 = max_axis_min_x2 + max_axis_length - max_axis_length/(NSPANS/ut_BoxCentre<BOX_TYPE>::scale);
        }
        else {
            // Search for nboxes/2 ordered index
            nth_index = indices+nboxes/2;
            //for (INT_TYPE spliti = 1; spliti < NSPLITS; ++spliti) {
            //    // The second condition should be redundant, but is just in case.
            //    if (left_counts[spliti] > max_count || spliti == NSPLITS-1) {
            //        min_pivotx2 = max_axis_min_x2 + spliti*max_axis_length/(NSPANS/ut_BoxCentre<BOX_TYPE>::scale);
            //        max_pivotx2 = max_axis_min_x2 + (spliti+1)*max_axis_length/(NSPANS/ut_BoxCentre<BOX_TYPE>::scale);
            //        break;
            //    }
            //}
        }
        nthElement<T>(boxes,indices,indices+nboxes,max_axis,nth_index);//,min_pivotx2,max_pivotx2);

        split_indices = nth_index;
        Box<T,NAXES> left_box(boxes[indices[0]]);
        for (SRC_INT_TYPE* left_indices = indices+1; left_indices < nth_index; ++left_indices) {
            left_box.combine(boxes[*left_indices]);
        }
        Box<T,NAXES> right_box(boxes[nth_index[0]]);
        for (SRC_INT_TYPE* right_indices = nth_index+1; right_indices < indices_end; ++right_indices) {
            right_box.combine(boxes[*right_indices]);
        }
        split_boxes[0] = left_box;
        split_boxes[1] = right_box;
    }
    else {
        const T pivotx2 = axis_min_x2 + (split_index+1)*axis_length/(NSPANS/ut_BoxCentre<BOX_TYPE>::scale);
        SRC_INT_TYPE* ppivot_start;
        SRC_INT_TYPE* ppivot_end;
        partitionByCentre(boxes,indices,indices+nboxes,max_axis,pivotx2,ppivot_start,ppivot_end);

        split_indices = indices + left_counts[split_index];

        // Ignoring roundoff error, we would have
        // split_indices >= ppivot_start && split_indices <= ppivot_end,
        // but it may not always be in practice.
        if (split_indices >= ppivot_start && split_indices <= ppivot_end) {
            split_boxes[0] = left_boxes[split_index];
            split_boxes[1] = right_boxes[split_index];
            return;
        }

        // Roundoff error changed the split, so we need to recompute the boxes.
        if (split_indices < ppivot_start) {
            split_indices = ppivot_start;
        }
        else {//(split_indices > ppivot_end)
            split_indices = ppivot_end;
        }

        // Emergency checks, just in case
        if (split_indices == indices) {
            ++split_indices;
        }
        else if (split_indices == indices_end) {
            --split_indices;
        }

        Box<T,NAXES> left_box(boxes[indices[0]]);
        for (SRC_INT_TYPE* left_indices = indices+1; left_indices < split_indices; ++left_indices) {
            left_box.combine(boxes[*left_indices]);
        }
        Box<T,NAXES> right_box(boxes[split_indices[0]]);
        for (SRC_INT_TYPE* right_indices = split_indices+1; right_indices < indices_end; ++right_indices) {
            right_box.combine(boxes[*right_indices]);
        }
        split_boxes[0] = left_box;
        split_boxes[1] = right_box;
    }
}

template<uint N>
template<uint PARALLEL_THRESHOLD, typename SRC_INT_TYPE>
void BVH<N>::adjustParallelChildNodes(INT_TYPE nparallel, UT_Array<Node>& nodes, Node& node, UT_Array<Node>* parallel_nodes, SRC_INT_TYPE* sub_indices) noexcept
{
	tbb::blocked_range<INT_TYPE>& r;
    //UTparallelFor(UT_BlockedRange<INT_TYPE>(0,nparallel), [&node,&nodes,&parallel_nodes,&sub_indices](const UT_BlockedRange<INT_TYPE>& r) {
        INT_TYPE counted_parallel = 0;
        INT_TYPE childi = 0;
        for (INT_TYPE taski = r.begin(), end = r.end(); taski < end; ++taski) {
            // First, find which child this is
            INT_TYPE sub_nboxes;
            for (; childi < N; ++childi) {
                sub_nboxes = sub_indices[childi+1]-sub_indices[childi];
                if (sub_nboxes >= PARALLEL_THRESHOLD) {
                    if (counted_parallel == taski) {
                        break;
                    }
                    ++counted_parallel;
                }
            }
            UT_ASSERT_P(counted_parallel == taski);

            const UT_Array<Node>& local_nodes = parallel_nodes[counted_parallel];
            INT_TYPE n = local_nodes.size();
            INT_TYPE local_nodes_start = Node::getInternalNum(node.child[childi])+1;
            ++counted_parallel;
            ++childi;

            for (INT_TYPE j = 0; j < n; ++j) {
                Node local_node = local_nodes[j];
                for (INT_TYPE childj = 0; childj < N; ++childj) {
                    INT_TYPE local_child = local_node.child[childj];
                    if (Node::isInternal(local_child) && local_child != Node::EMPTY) {
                        local_child += local_nodes_start;
                        local_node.child[childj] = local_child;
                    }
                }
                nodes[local_nodes_start+j] = local_node;
            }
        }
    //}, 0, 1);
}

template<uint N>
template<typename T,typename BOX_TYPE,typename SRC_INT_TYPE>
void BVH<N>::nthElement(const BOX_TYPE* boxes, SRC_INT_TYPE* indices, const SRC_INT_TYPE* indices_end, const uint axis, SRC_INT_TYPE*const nth) noexcept {//, const T min_pivotx2, const T max_pivotx2) noexcept {
    while (true) {
        // Choose median of first, middle, and last as the pivot
        T pivots[3] = {
            utBoxCenter(boxes[indices[0]], axis),
            utBoxCenter(boxes[indices[(indices_end-indices)/2]], axis),
            utBoxCenter(boxes[*(indices_end-1)], axis)
        };
        if (pivots[0] < pivots[1]) {
            const T temp = pivots[0];
            pivots[0] = pivots[1];
            pivots[1] = temp;
        }
        if (pivots[0] < pivots[2]) {
            const T temp = pivots[0];
            pivots[0] = pivots[2];
            pivots[2] = temp;
        }
        if (pivots[1] < pivots[2]) {
            const T temp = pivots[1];
            pivots[1] = pivots[2];
            pivots[2] = temp;
        }
        T mid_pivotx2 = pivots[1];
#if 0
        // We limit the pivot, because we know that the true value is between min and max
        if (mid_pivotx2 < min_pivotx2) {
            mid_pivotx2 = min_pivotx2;
        }
        else if (mid_pivotx2 > max_pivotx2) {
            mid_pivotx2 = max_pivotx2;
        }
#endif
        SRC_INT_TYPE* pivot_start;
        SRC_INT_TYPE* pivot_end;
        partitionByCentre(boxes,indices,indices_end,axis,mid_pivotx2,pivot_start,pivot_end);
        if (nth < pivot_start) {
            indices_end = pivot_start;
        }
        else if (nth < pivot_end) {
            // nth is in the middle of the pivot range,
            // which is in the right place, so we're done.
            return;
        }
        else {
            indices = pivot_end;
        }
        if (indices_end <= indices+1) {
            return;
        }
    }
}

template<uint N>
template<typename T,typename BOX_TYPE,typename SRC_INT_TYPE>
void BVH<N>::partitionByCentre(const BOX_TYPE* boxes, SRC_INT_TYPE*const indices, const SRC_INT_TYPE*const indices_end, const uint axis, const T pivotx2, SRC_INT_TYPE*& ppivot_start, SRC_INT_TYPE*& ppivot_end) noexcept {
    // TODO: Consider parallelizing this!

    // First element >= pivot
    SRC_INT_TYPE* pivot_start = indices;
    // First element > pivot
    SRC_INT_TYPE* pivot_end = indices;

    // Loop through forward once
    for (SRC_INT_TYPE* psrc_index = indices; psrc_index != indices_end; ++psrc_index) {
        const T srcsum = utBoxCenter(boxes[*psrc_index], axis);
        if (srcsum < pivotx2) {
            if (psrc_index != pivot_start) {
                if (pivot_start == pivot_end) {
                    // Common case: nothing equal to the pivot
                    const SRC_INT_TYPE temp = *psrc_index;
                    *psrc_index = *pivot_start;
                    *pivot_start = temp;
                }
                else {
                    // Less common case: at least one thing equal to the pivot
                    const SRC_INT_TYPE temp = *psrc_index;
                    *psrc_index = *pivot_end;
                    *pivot_end = *pivot_start;
                    *pivot_start = temp;
                }
            }
            ++pivot_start;
            ++pivot_end;
        }
        else if (srcsum == pivotx2) {
            // Add to the pivot area
            if (psrc_index != pivot_end) {
                const SRC_INT_TYPE temp = *psrc_index;
                *psrc_index = *pivot_end;
                *pivot_end = temp;
            }
            ++pivot_end;
        }
    }
    ppivot_start = pivot_start;
    ppivot_end = pivot_end;
}

#if 0
template<uint N>
void BVH<N>::debugDump() const {
    printf("\nNode 0: {\n");
    UT_WorkBuffer indent;
    indent.append(80, ' ');
    UT_Array<INT_TYPE> stack;
    stack.append(0);
    stack.append(0);
    while (!stack.isEmpty()) {
        int depth = stack.size()/2;
        if (indent.length() < 4*depth) {
            indent.append(4, ' ');
        }
        INT_TYPE cur_nodei = stack[stack.size()-2];
        INT_TYPE cur_i = stack[stack.size()-1];
        if (cur_i == N) {
            printf(indent.buffer()+indent.length()-(4*(depth-1)));
            printf("}\n");
            stack.removeLast();
            stack.removeLast();
            continue;
        }
        ++stack[stack.size()-1];
        Node& cur_node = myRoot[cur_nodei];
        INT_TYPE child_nodei = cur_node.child[cur_i];
        if (Node::isInternal(child_nodei)) {
            if (child_nodei == Node::EMPTY) {
                printf(indent.buffer()+indent.length()-(4*(depth-1)));
                printf("}\n");
                stack.removeLast();
                stack.removeLast();
                continue;
            }
            INT_TYPE internal_node = Node::getInternalNum(child_nodei);
            printf(indent.buffer()+indent.length()-(4*depth));
            printf("Node %u: {\n", uint(internal_node));
            stack.append(internal_node);
            stack.append(0);
            continue;
        }
        else {
            printf(indent.buffer()+indent.length()-(4*depth));
            printf("Tri %u\n", uint(child_nodei));
        }
    }
}
#endif

} // UT namespace
} // End HDK_Sample namespace
#endif
