#pragma once

//#include <Eigen/Core>
#include <Eigen/Dense>
//#include <Eigen/SVD>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"

#include "Mesh.h"

#include <vector>
#include <limits>


#define EPS 1.0e-10
#define MIN_VAL -FLT_MAX
#define MAX_VAL FLT_MAX

using namespace glm; 
using namespace Eigen;

class Geometry {
public: 
	static void ComputeSpatialProperties(std::vector<float>* verts) {
		const int numVerts = ((int)verts->size()) / 3;
		vec3 minPoint(MIN_VAL, MIN_VAL, MIN_VAL);
		vec3 maxPoint(MAX_VAL, MAX_VAL, MAX_VAL);
		vec3 centroid(0.0f);
				
		MatrixXf A(numVerts, 3);

		for (int i = 0; i < numVerts; i++) {

			float x = (*verts)[i * 3];
			float y = (*verts)[i * 3 + 1];
			float z = (*verts)[i * 3 + 2];
			if (x > minPoint.x) { minPoint.x = x; }
			if (y > minPoint.y) { minPoint.y = y; }
			if (z > minPoint.z) { minPoint.z = z; }

			centroid.x += x;
			centroid.y += y;
			centroid.z += z;

			A.row(i) << x, y, z;
		}

		int ti = 1;

		



	}


};


class Intersect {

public:
	bool rayTriangleIntersect(const vec3 &orig, const vec3 &dir,
		const vec3 &v0, const vec3 &v1, const vec3 &v2,
		float &t, float &u, float &v) {
		// compute plane's normal
		vec3 v0v1 = v1 - v0;
		vec3 v0v2 = v2 - v0;
		// no need to normalize
		vec3 N = cross(v0v1, v0v2); // N 
		float denom = dot(N, N);

		// Step 1: finding P

		// check if ray and plane are parallel ?
		float NdotRayDirection = dot(N, dir);
		if (fabs(NdotRayDirection) < EPS) // almost 0 
			return false; // they are parallel so they don't intersect ! 

		// compute d parameter using equation 2
		float d = dot(N, v0);

		// compute t (equation 3)
		t = (dot(N, orig) + d) / NdotRayDirection;
		// check if the triangle is in behind the ray
		if (t < 0) return false; // the triangle is behind 

		// compute the intersection point using equation 1
		vec3 P = orig + t * dir;

		// Step 2: inside-outside test
		vec3 C; // vector perpendicular to triangle's plane 

		// edge 0
		vec3 edge0 = v1 - v0;
		vec3 vp0 = P - v0;
		C = cross(edge0, vp0);
		if (dot(N, C) < 0) return false; // P is on the right side 

		// edge 1
		vec3 edge1 = v2 - v1;
		vec3 vp1 = P - v1;
		C = cross(edge1, vp1);
		if ((u = dot(N, C)) < 0)  return false; // P is on the right side 

		// edge 2
		vec3 edge2 = v0 - v2;
		vec3 vp2 = P - v2;
		C = cross(edge2, vp2);
		if ((v = dot(N, C)) < 0) return false; // P is on the right side; 

		u /= denom;
		v /= denom;

		return true; // this ray hits the triangle 
	}

	static bool RayTriangle(Face* f, vec3 rayOrigin, vec3 rayDir, float &t) {

		vec3 normal = f->normal;
		float area2 = normal.length();

		// Step 1: finding P

		// check if ray and plane are parallel ?
		float NdotRayDirection = dot(normal, rayDir);
		if (fabs(NdotRayDirection) < EPS) // almost 0 
			return false; // they are parallel so they don't intersect ! 

		// compute d parameter using equation 2
		float d = dot(normal, f->p0);

		// compute t (equation 3)
		t = (dot(normal, rayOrigin) + d) / NdotRayDirection;
		// check if the triangle is in behind the ray
		if (t < 0) return false; // the triangle is behind 

		// compute the intersection point using equation 1
		vec3 P = rayOrigin + t * rayDir;

		// Step 2: inside-outside test
		vec3 C; // vector perpendicular to triangle's plane 

		// edge 0
		vec3 edge0 = f->p1 - f->p0;
		vec3 vp0 = P - f->p0;
		C = cross(edge0, vp0);
		if (dot(normal, C) < 0) return false; // P is on the right side 

		// edge 1
		vec3 edge1 = f->p2 - f->p1;
		vec3 vp1 = P - f->p1;
		C = cross(edge1, vp1);
		if (dot(normal, C) < 0) return false; // P is on the right side 

		// edge 2
		vec3 edge2 = f->p0 - f->p2;
		vec3 vp2 = P - f->p2;
		C = cross(edge2, vp2);
		if (dot(normal, C) < 0) return false; // P is on the right side 

		return true; // this ray hits the triangle 
	}


	static bool RayPlane(const vec3 &n, const vec3 &planeOrigin, const vec3 &rayOrigin, const vec3 &rayDir, float &t) {
		// assuming vectors are all normalized
		float denom = dot(n, rayDir);
		if (denom > EPS) {
			vec3 diff = planeOrigin - rayOrigin;
			t = dot(diff, n) / denom;
			return (t >= 0);
		}

		return false;
	}

	static bool RayDisk(const vec3 &n, const vec3 &planeOrigin, const float &radius, const vec3 &rayOrigin, const vec3 &rayDir) {
		float t = 0;
		if (RayPlane(n, planeOrigin, rayOrigin, rayDir, t)) {
			vec3 p = rayOrigin + rayDir * t;
			vec3 v = p - planeOrigin;
			float d2 = dot(v, v);
			return (d2 <= (radius * radius)); //We compare the squared distance to avoid taking a sqrt here
		}

		return false;
	}

};