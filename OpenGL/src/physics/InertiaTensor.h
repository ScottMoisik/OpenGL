#pragma once
/*******************************************************
		*                                                      *
	*  volInt.c                                            *
	*                                                      *
	*  This code computes volume integrals needed for      *
	*  determining mass properties of polyhedral bodies.   *
	*                                                      *
	*  For more information, see the accompanying README   *
	*  file, and the paper                                 *
	*                                                      *
	*  Brian Mirtich, "Fast and Accurate Computation of    *
	*  Polyhedral Mass Properties," journal of graphics    *
	*  tools, volume 1, number 2, 1996.                    *
	*                                                      *
	*  This source code is public domain, and may be used  *
	*  in any way, shape or form, free of charge.          *
	*                                                      *
	*  Copyright 1995 by Brian Mirtich                     *
	*                                                      *
	*  mirtich@cs.berkeley.edu                             *
	*  http://www.cs.berkeley.edu/~mirtich                 *
		*                                                      *
	*******************************************************/

	/*
		Revision history
		26 Jan 1996	Program creation.
		 3 Aug 1996	Corrected bug arising when polyhedron density
				is not 1.0.  Changes confined to function main().
				Thanks to Zoran Popovic for catching this one.
		27 May 1997     Corrected sign error in translation of inertia
						product terms to center of mass frame.  Changes
				confined to function main().  Thanks to
				Chris Hecker.
	*/


	/*
	   ============================================================================
	   constants
	   ============================================================================
	*/



#include "Mesh.h"

#define MAX_VERTS 100     /* maximum number of polyhedral vertices */
#define MAX_FACES 100     /* maximum number of polyhedral faces */
#define MAX_POLYGON_SZ 10 /* maximum number of verts per polygonal face */

#define X 0
#define Y 1
#define Z 2

	/*
	   ============================================================================
	   macros
	   ============================================================================
	*/

#define SQR(x) ((x)*(x))
#define CUBE(x) ((x)*(x)*(x))

class InertiaTensor {

	/*
	   ============================================================================
	   data structures
	   ============================================================================
	*/

	typedef struct {
		int numVerts;
		double norm[3];
		double w;
		int verts[MAX_POLYGON_SZ];
		struct polyhedron *poly;
	} FACE;

	typedef struct polyhedron {
		int numVerts, numFaces;
		double verts[MAX_VERTS][3];
		FACE faces[MAX_FACES];
	} POLYHEDRON;


	/*
	   ============================================================================
	   globals
	   ============================================================================
	*/

	static int A;   /* alpha */
	static int B;   /* beta */
	static int C;   /* gamma */

	/* projection integrals */
	static float P1, Pa, Pb, Paa, Pab, Pbb, Paaa, Paab, Pabb, Pbbb;

	/* face integrals */
	static float Fa, Fb, Fc, Faa, Fbb, Fcc, Faaa, Fbbb, Fccc, Faab, Fbbc, Fcca;

	/* volume integrals */
	static float T0, T1[3], T2[3], TP[3];


	/*
	   ============================================================================
	   compute mass properties
	   ============================================================================
	*/
public:
	/* compute various integrations over projection of face */
	static void compProjectionIntegrals(Face *f);
	static void compFaceIntegrals(Face *f);
	static void compVolumeIntegrals(std::shared_ptr<Mesh> mesh);


	/*
	   ============================================================================
	   main
	   ============================================================================
	*/

	InertiaTensor() {};
	~InertiaTensor() {};
	struct MassProps {
		glm::mat3 Ibody; //Inertia tensor in body space
		float mass;
		glm::vec3 com; //Center of mass
	};

	static MassProps Compute(std::shared_ptr<Mesh> mesh, float density);
};