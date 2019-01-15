#pragma once
//Assuming your trimesh is closed(whether convex or not) there is a way!
//
//As dmckee points out, the general approach is building tetrahedrons from each surface triangle, then applying the obvious math to total up the mass and moment contributions from each tet.The trick comes in when the surface of the body has concavities that make internal pockets when viewed from whatever your reference point is.
//
//So, to get started, pick some reference point(the origin in model coordinates will work fine), it doesn't even need to be inside of the body. For every triangle, connect the three points of that triangle to the reference point to form a tetrahedron. Here's the trick : use the triangle's surface normal to figure out if the triangle is facing towards or away from the reference point (which you can find by looking at the sign of the dot product of the normal and a vector pointing at the centroid of the triangle). If the triangle is facing away from the reference point, treat its mass and moment normally, but if it is facing towards the reference point (suggesting that there is open space between the reference point and the solid body), negate your results for that tet.
//
//Effectively what this does is over - count chunks of volume and then correct once those areas are shown to be not part of the solid body.If a body has lots of blubbery flanges and grotesque folds(got that image ? ), a particular piece of volume may be over - counted by a hefty factor, but it will be subtracted off just enough times to cancel it out if your mesh is closed.Working this way you can even handle internal bubbles of space in your objects(assuming the normals are set correctly).On top of that, each triangle can be handled independently so you can parallelize at will.Enjoy!
//
//Afterthought : You might wonder what happens when that dot product gives you a value at or near zero.This only happens when the triangle face is parallel(its normal is perpendicular) do the direction to the reference point -- which only happens for degenerate tets with small or zero area anyway.That is to say, the decision to add or subtract a tet's contribution is only questionable when the tet wasn't going to contribute anything anyway.



//http://number-none.com/blow/inertia/index.html

#include "Mesh.h"

#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"

struct InertialProps {
	glm::mat3 C; //Covariance matrix
	float mass; 
	glm::vec3 com; //Center of mass
};


// Tonon, F. (2005). Explicit Exact Formulas for the 3-D Tetrahedron Inertia Tensor in Terms of its Vertex Coordinates. Journal of Mathematics and Statistics, 1(1), 8–11. https://doi.org/10.3844/jmssp.2005.8.11
InertialProps ComputeInertiaProperties(float density) {
	using namespace glm;

	vec3 A1 = vec3(8.33220, -11.86875, 0.93355);
	vec3 A2 = vec3(0.75523, 5.000000, 16.37072);
	vec3 A3 = vec3(52.61236, 5.000000, -5.38580);
	vec3 A4 = vec3(2.000000, 5.000000, 3.000000);
	
	vec4 avg = vec4(1.0f / 4.0f);
	vec3 centroid = A1 + A2 + A3 + A4;
	centroid *= 0.25;
	
	auto poly = [](vec4 v) { return 
		v[0] * v[0] + v[0] * v[1] + 
		v[1] * v[1] + v[0] * v[2] + v[1] * v[2] + 
		v[2] * v[2] + v[0] * v[3] + v[1] * v[3] + v[2] * v[3] + v[3] * v[3]; };
	
	vec3 A1c = A1 - centroid;
	vec3 A2c = A2 - centroid;
	vec3 A3c = A3 - centroid;
	vec3 A4c = A4 - centroid;
	vec4 xCol = vec4(A1c[0], A2c[0], A3c[0], A4c[0]);
	vec4 yCol = vec4(A1c[1], A2c[1], A3c[1], A4c[1]);
	vec4 zCol = vec4(A1c[2], A2c[2], A3c[2], A4c[2]);

	mat3 A = mat3(A2c - A1c, A3c - A1c, A4c - A1c);
	float detJ = determinant(A);
	/*
	float detJalt = //GLM is column major, access is [col][row]!!!
		(A[0][0] * (A[1][1] * A[2][2] - A[2][1] * A[1][2])) -
		(A[1][0] * (A[0][1] * A[2][2] - A[2][1] * A[0][2])) +
		(A[2][0] * (A[0][1] * A[1][2] - A[1][1] * A[0][2]));
		*/

	float mass = density * abs(detJ);
	float a = mass * (poly(yCol) + poly(zCol)) / 60.0f;
	float b = mass * (poly(xCol) + poly(zCol)) / 60.0f;
	float c = mass * (poly(xCol) + poly(yCol)) / 60.0f;

	auto polyPrime = [](vec4 v, vec4 w) { return 
		2 * v[0] * w[0] +     v[1] * w[0] +		v[2] * w[0] +		v[3] * w[0] + 
		    v[0] * w[1] + 2 * v[1] * w[1] +		v[2] * w[1] +		v[3] * w[1] + 
			v[0] * w[2] +     v[1] * w[2] + 2 * v[2] * w[2] +		v[3] * w[2] +
			v[0] * w[3] +     v[1] * w[3] +		v[2] * w[3] +	2 * v[3] * w[3]; };

	float aPrime = mass * (polyPrime(yCol, zCol)) / 120.0f;
	float bPrime = mass * (polyPrime(xCol, zCol)) / 120.0f;
	float cPrime = mass * (polyPrime(xCol, yCol)) / 120.0f;


	int q = 1;
	return { A, 0.0f, vec3(0.0f) };

}


InertialProps ComputeIntertiaProperties(Mesh& mesh, float density) {

	std::vector<float>& pos = mesh.GetPositions();
	std::vector<unsigned int>& inds = mesh.GetIndices();

	// Averaging vector
	glm::vec3 avg = glm::vec3(1.0 / 4.0f); //Mean of four vertex coordinates

	glm::mat3 I = glm::mat3(1.0f);

	//Canonical tetrahedral covariance matrix
	glm::mat3 C_canonical = glm::mat3(
		1.0f / 60.0f, 1.0f / 120.0f, 1.0 / 120.0f,
		1.0f / 120.0f, 1.0f / 60.0f, 1.0 / 120.0f,
		1.0f / 120.0f, 1.0 / 120.0f, 1.0f / 60.0f);

	//Accumulation values
	float massTotal = 0.0f;
	glm::vec3 centerOfMass = glm::vec3(0.0f);
	glm::mat3 C = glm::mat3(0.0f); //Covariance matrix

	//For each triangle, form a tetrahedron and compute its inertial properties
	int numTriangles = inds.size() / 3;
	for (int i = 0; i < numTriangles; i++) {
		int p1 = inds[i * 3];
		int p2 = inds[i * 3 + 1];
		int p3 = inds[i * 3 + 2];
		glm::mat3 A = glm::mat3(
			pos[p1 * 3], pos[p1 * 3 + 1], pos[p1 * 3 + 2],
			pos[p2 * 3], pos[p2 * 3 + 1], pos[p2 * 3 + 2],
			pos[p3 * 3], pos[p3 * 3 + 1], pos[p3 * 3 + 2]);

		float detA = glm::determinant(A);
		C += detA * A * C_canonical * glm::transpose(A);

		float mass = (density * (1.0f / 6.0f) * detA);
		
		glm::vec3 avgPos = (A * avg);
		centerOfMass = ((centerOfMass * massTotal) + (avgPos * mass)) / (massTotal + mass);
		
		massTotal += mass;

	}


	//mass should be 8.37758041 (density = 2, radius = 1)
	return { C, massTotal, centerOfMass };

}