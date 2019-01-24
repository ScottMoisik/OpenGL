#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"

#include "Mesh.h"

#define EPS 1.0e-10

using namespace glm;
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

	bool RayTriangleIntersect(
		const glm::vec3 &orig, const glm::vec3 &dir,
		const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2,
		float &t, float &u, float &v) {

		glm::vec3 v0v1 = v1 - v0;
		glm::vec3 v0v2 = v2 - v0;
		glm::vec3 pvec = glm::cross(dir, v0v2);
		float det = glm::dot(v0v1, pvec);

		// if the determinant is negative the triangle is backfacing
		// if the determinant is close to 0, the ray misses the triangle
		if (det < EPS) return false;

		// ray and triangle are parallel if det is close to 0
		if (fabs(det) < EPS) return false;

		float invDet = 1 / det;

		glm::vec3 tvec = orig - v0;
		u = glm::dot(tvec, pvec) * invDet;
		if (u < 0 || u > 1) return false;

		glm::vec3 qvec = glm::cross(tvec, v0v1);
		v = glm::dot(dir, qvec) * invDet;
		if (v < 0 || u + v > 1) return false;

		t = glm::dot(v0v2, qvec) * invDet;

		return true;
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