#include "Geometry.h"


#include "Eigen/Core"
#include "Eigen/Dense"
//#include <Eigen/SVD>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"


#include <iostream>
#include <limits>


#define MIN_VAL -FLT_MAX
#define MAX_VAL FLT_MAX


using namespace Eigen;


void ComputeSpatialProperties(std::vector<float>* verts) {
	const int numVerts = ((int)verts->size()) / 3;
	glm::vec3 minPoint(MIN_VAL, MIN_VAL, MIN_VAL);
	glm::vec3 maxPoint(MAX_VAL, MAX_VAL, MAX_VAL);
	glm::vec3 centroid(0.0f);

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

	//https://stats.stackexchange.com/questions/134282/relationship-between-svd-and-pca-how-to-use-svd-to-perform-pca
	//std::cout << A << std::endl;

	JacobiSVD<MatrixXf> svd(A, ComputeThinV);
	std::cout << "Its singular values are:" << std::endl << svd.singularValues() << std::endl;
	std::cout << "Its right singular vectors are the columns of the thin V matrix:" << std::endl << svd.matrixV() << std::endl;
	int ti = 1;





}


