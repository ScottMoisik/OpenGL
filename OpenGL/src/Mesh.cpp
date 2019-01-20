#include "Mesh.h"

#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath>

#include "VertexBufferLayout.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"


Mesh::Mesh(const std::string& filepath, unsigned int numInstances) : m_Filepath(filepath), m_NumInstances(numInstances) {
	ParseMeshFile(filepath);
	SetupMesh();
}

Mesh::~Mesh() {
	//TODO: Delete stuff??
	//GLCall(glDeleteTextures(1, &m_RendererID));
}


void Mesh::Update(float deltaTime, float scale, glm::vec3 trans, float angularVel, glm::vec3 rotAxis) {

	// Define instance matrices
	m_InstanceModelMatrices.clear();
	m_InstanceMVPMatrices.clear();

	for (unsigned int i = 0; i < m_NumInstances; i++) {
		m_InstanceModelMatrices.push_back(glm::translate(glm::mat4(1.0f), trans + glm::vec3(trans.x*(float)i, trans.x*(float)i, trans.z*(float)i)));
		m_InstanceMVPMatrices.push_back(glm::rotate(glm::mat4(1.0f), angularVel*((float)deltaTime), rotAxis) * glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, scale)));
	}

	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[WORLD_MAT_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * m_NumInstances, &m_InstanceModelMatrices[0], GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[WVP_MAT_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * m_NumInstances, &m_InstanceMVPMatrices[0], GL_DYNAMIC_DRAW);

}

void Mesh::Draw(const Shader& shader) {
	shader.Bind();
	m_VAO->Bind();
	glDrawElementsInstanced(GL_TRIANGLES, m_VertexIndices.size(), GL_UNSIGNED_INT, 0, m_NumInstances);
}

/* Uses the structure of arrays approach (one array for each attribute type) */
void Mesh::SetupMesh() {
	// Create face structure for the mesh
	CreateFaces();

	// Ensure that the winding order is correct 
	FixWinding();



	/* Setup vertex array object */
	m_VAO = std::make_unique<VertexArray>();
	m_VAO->Bind();

	/* Create the array buffers for the vertex atttributes */
	glGenBuffers(ARRAY_SIZE(m_Buffers), m_Buffers);

	/* Reserve space in the vectors for the vertex attributes and indices */
	// Generate and populate the buffers with vertex attributes and the indices
	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[POS_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(m_Positions[0]) * m_Positions.size(), &m_Positions[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(POSITION_LOCATION);
	glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[NORMAL_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(m_Normals[0]) * m_Normals.size(), &m_Normals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(NORMAL_LOCATION);
	glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);

	/* Setup texture coordinate array buffer */
	if (m_TextureCoordinates.size() > 0) {
		glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[TEXCOORD_VB]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(m_TextureCoordinates[0]) * m_TextureCoordinates.size(), &m_TextureCoordinates[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(TEXTURE_LOCATION);
		glVertexAttribPointer(TEXTURE_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[INDEX_BUFFER]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_VertexIndices[0]) * m_VertexIndices.size(), &m_VertexIndices[0], GL_STATIC_DRAW);

	/* Bind the array buffer used for feeding instance matrices to the mesh shader */
	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[WORLD_MAT_VB]);

	/* Set the vertex attributes for the instanced matrices */
	GLsizei vec4Size = sizeof(glm::vec4);
	for (int i = 0; i < 4; i++) {
		int attributePosition = WORLD_LOCATION + i;
		glEnableVertexAttribArray(attributePosition);
		glVertexAttribPointer(attributePosition, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(i * vec4Size));
		glVertexAttribDivisor(attributePosition, 1);
	}

	/* Bind the array buffer used for feeding instance matrices to the mesh shader */
	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[WVP_MAT_VB]);

	/* Set the vertex attributes for the instanced matrices */
	for (int i = 0; i < 4; i++) {
		int attributePosition = MVP_LOCATION + i;
		glEnableVertexAttribArray(attributePosition);
		glVertexAttribPointer(attributePosition, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(i * vec4Size));
		glVertexAttribDivisor(attributePosition, 1);
	}

	m_VAO->Unbind();
}



void Mesh::CreateFaces() {
	using namespace glm;

	// Create faces
	int numFaces = m_VertexIndices.size() / 3;
	for (int fIdx = 0; fIdx < numFaces; fIdx++) {
		int i = m_VertexIndices[fIdx * 3];
		int j = m_VertexIndices[fIdx * 3 + 1];
		int k = m_VertexIndices[fIdx * 3 + 2];
		vec3 p0 = vec3(m_Positions[i * 3], m_Positions[i * 3 + 1], m_Positions[i * 3 + 2]);
		vec3 p1 = vec3(m_Positions[j * 3], m_Positions[j * 3 + 1], m_Positions[j * 3 + 2]);
		vec3 p2 = vec3(m_Positions[k * 3], m_Positions[k * 3 + 1], m_Positions[k * 3 + 2]);
		vec3 normal = normalize(cross(p1 - p0, p2 - p1));

		float offset = -normal.x * p0.x - normal.y * p0.y - normal.z * p0.z;

		Face f = { p0, p1, p2, normal, i, j, k, offset };
		f.verts.insert(f.verts.begin(), { p0, p1, p2 }); //Add the positions to a vector for the purposes of inertia tensor computation (see InertiaTensor.h)
		m_Faces.push_back(f);

	}

	// Determine connectivity of faces by using the positions (since indices may not necessarily be shared if separate vertices between adjacent faces are used for lighting purposes, as in the case of the cube)
	for (Face& face1 : m_Faces) {

		// Search through all faces and find all edge neighbours (vertex neighbours are ignored)
		for (Face& face2 : m_Faces) {
			if (&face1 != &face2) {
				int p0Match = (face1.p0 == face2.p0 || face1.p0 == face2.p1 || face1.p0 == face2.p2);
				int p1Match = (face1.p1 == face2.p0 || face1.p1 == face2.p1 || face1.p1 == face2.p2);
				int p2Match = (face1.p2 == face2.p0 || face1.p2 == face2.p1 || face1.p2 == face2.p2);

				if (p0Match + p1Match + p2Match == 2) {
					face1.neighbours.push_back(&face2);
				}

				//If three neighbours have been found, stop as there is no more point in searching
				if (face1.neighbours.size() == 3) {
					break;
				}
			}
		}
	}
}

void Mesh::FixWinding() {
	using namespace glm;


	//Systematically search all faces and correct the winding using the first face as the standard
	Face* face = &m_Faces[0];
	std::vector<Face*> checkedFaces;
	std::vector<Face*> searchFaces;
	checkedFaces.push_back(face);

	searchFaces.insert(searchFaces.end(), face->neighbours.begin(), face->neighbours.end());
	Face* searchFace;

	while (searchFaces.size() > 0) {
		searchFace = searchFaces[0];

		if (std::find(checkedFaces.begin(), checkedFaces.end(), searchFace) == checkedFaces.end()) {
			int p0Match = (face->p0 == searchFace->p0) + 2 * (face->p0 == searchFace->p1) + 3 * (face->p0 == searchFace->p2);
			int p1Match = (face->p1 == searchFace->p0) + 2 * (face->p1 == searchFace->p1) + 3 * (face->p1 == searchFace->p2);
			int p2Match = (face->p2 == searchFace->p0) + 2 * (face->p2 == searchFace->p1) + 3 * (face->p2 == searchFace->p2);

			int first = 0;
			int second = 0;

			if (p0Match > 0 && p1Match > 0) {
				//0-1 edge
				first = p0Match;
				second = p1Match;
			} else if (p1Match > 0 && p2Match > 0) {
				//1-2 edge
				first = p1Match;
				second = p2Match;
			} else {
				//2-0 edge
				first = p2Match;
				second = p0Match;
			}

			//If any of these orderings occur, it means that the vertex winding is not consistent with the current face
			if ((first == 2 && second == 3) || (first == 2 && second == 3) || (first == 1 && second == 2)) {
				//Correct the winding by swapping the ordering of the first two vertices (any pair will do) in the search face
				int tempIndex = searchFace->i0;
				searchFace->i0 = searchFace->i1;
				searchFace->i1 = tempIndex;

				//glm::vec3 tempPos = searchFace->p0;
				//searchFace->p0 = searchFace->p1;
				//searchFace->p1 = tempPos;

				searchFace->normal = glm::normalize(glm::cross(searchFace->p1 - searchFace->p0, searchFace->p2 - searchFace->p0));
			}

			//Add the search face to the list of checked faces
			checkedFaces.push_back(searchFace);

			//Add the search faces neighbours to the list of search faces (if the neighbour is not already in the list)
			for (Face* neighbour : searchFace->neighbours) {
				bool checkedFlag = (std::find(checkedFaces.begin(), checkedFaces.end(), neighbour) == checkedFaces.end());
				bool searchFlag = (std::find(searchFaces.begin(), searchFaces.end(), neighbour) == searchFaces.end());
				if (checkedFlag && searchFlag) {
					searchFaces.push_back(neighbour);
				}
			}

			ptrdiff_t pos = std::find(searchFaces.begin(), searchFaces.end(), searchFace) - searchFaces.begin();
			if (pos < (ptrdiff_t)searchFaces.size()) {
				searchFaces.erase(searchFaces.begin() + pos);
			}
		}
	}

	//Replace vertex indices and normals in the mesh data structure
	for (int fIdx = 0; fIdx < (int)m_Faces.size(); fIdx++) {
		m_VertexIndices[fIdx * 3] = m_Faces[fIdx].i0;
		m_VertexIndices[fIdx * 3 + 1] = m_Faces[fIdx].i1;
		m_VertexIndices[fIdx * 3 + 2] = m_Faces[fIdx].i2;

		for (int nIdx = 0; nIdx < 3; nIdx++) {
			int j = m_VertexIndices[(fIdx * 3) + nIdx];
			m_Normals[j * 3] = m_Faces[fIdx].normal.x;
			m_Normals[j * 3 + 1] = m_Faces[fIdx].normal.y;
			m_Normals[j * 3 + 2] = m_Faces[fIdx].normal.z;
		}
	}

	int ththt = 1;
}




enum AttributeType { NONE, POSITION, VERTEX_NORMAL, TEXTURE_UV_COORD, FACE };

class VertexAttribute {
public:
	VertexAttribute(const std::string& type) {
		if (type.find("v") != std::string::npos && type.size() == 1) {
			at = POSITION; //Positions are given as "v" in Wavefront .OBJ 
		} else if (type.find("vn") != std::string::npos) {
			at = VERTEX_NORMAL;
		} else if (type.find("vt") != std::string::npos) {
			at = TEXTURE_UV_COORD;
		} else if (type.find("f") != std::string::npos) {
			at = FACE;
		}
	}
	~VertexAttribute() {}
	bool IsType(const AttributeType type) const { return type == at; }
private:
	AttributeType at;
};

void Mesh::ParseMeshFile(const std::string& filepath) {
	using namespace std::literals::chrono_literals;
	auto start = std::chrono::high_resolution_clock::now();
	std::ifstream stream(filepath);

	std::vector<float> normalsTemp;
	std::vector<float> texCoordTemp;

	//if (!stream) {
		/* Set dimensions to zero so that the check for dimensionality will occur */
	m_Dimensions = 0;
	//std::vector<Texture> m_Textures;
	std::string line;

	while (getline(stream, line)) {
		if (line.find("mtllib") != std::string::npos) {
			//m_MaterialFilepath = line;
		} else if (line.find("#") != std::string::npos) {

		} else {
			std::vector<std::string> tokens;
			std::string token;
			std::istringstream tokenStream(line);
			std::getline(tokenStream, token, ' ');
			VertexAttribute at = VertexAttribute(token);
			int vertexCount = 1; //Used to check if we have a triangle or a quad (and hence need to add an additional vertex to convert to two triangles)

			while (std::getline(tokenStream, token, ' ')) {

				try {
					if (at.IsType(AttributeType::POSITION)) {
						m_Positions.push_back(std::stof(token));
					} else if (at.IsType(AttributeType::VERTEX_NORMAL)) {
						normalsTemp.push_back(std::stof(token));
					} else if (at.IsType(AttributeType::TEXTURE_UV_COORD)) {
						texCoordTemp.push_back(std::stof(token));
					} else if (at.IsType(AttributeType::FACE)) {
						/* Check if this is the 4th vertex, and, if so, copy the first and third previous indices to complete the triangle */
						if (vertexCount > 3) {
							/* TO convert a quad to two triangles, we first need to copy the previous indices */
							m_VertexIndices.push_back(m_VertexIndices.back());
							m_NormalIndices.push_back(m_NormalIndices.back());
							if (m_TextureIndices.size() > 0) { m_TextureIndices.push_back(m_TextureIndices.back()); }
						}

						int firstSlash = token.find("/");
						int secondSlash = token.find("/", firstSlash + 1);

						/* Store indices, subtracting 1 because Wavefront .OBJ indices start at 1*/
						std::string vertexIndex = token.substr(0, firstSlash);
						m_VertexIndices.push_back(std::stoi(vertexIndex) - 1);

						std::string normalIndex = token.substr(secondSlash + 1);
						m_NormalIndices.push_back(std::stoi(normalIndex) - 1);

						if (secondSlash - firstSlash != 1) {
							std::string textureIndex = token.substr(firstSlash + 1, firstSlash + (secondSlash - firstSlash) - 1);
							m_TextureIndices.push_back(std::stoi(textureIndex) - 1);
						}

						/* Check if this is the 4th vertex, and, if so, copy the first and third previous indices to complete the triangle */
						if (vertexCount > 3) {
							/* To convert a quad to two triangles, we then need to copy the first indices that we found in this line */
							m_VertexIndices.push_back(m_VertexIndices[m_VertexIndices.size() - 5]);
							m_NormalIndices.push_back(m_NormalIndices[m_NormalIndices.size() - 5]);
							if (m_TextureIndices.size() > 0) { m_TextureIndices.push_back(m_TextureIndices[m_TextureIndices.size() - 5]); }
						}

						vertexCount++;
					}
				}
				catch (...) {
					std::cout << "Could not parse string: " << token << std::endl;
				}
			}

			/* Determine dimensionality using first position attribute that we encounter */
			if (m_Dimensions == 0 && at.IsType(AttributeType::POSITION)) {
				m_Dimensions = (int)m_Positions.size();
			}

			//std::cout << std::endl;

		}
	}


	/* Map position and normal indices based on the face data */
	for (int i = 0; i < (int)m_VertexIndices.size(); i++) {
		if (m_VertexIndexMap_PositionNormal.find(m_VertexIndices[i]) == m_VertexIndexMap_PositionNormal.end()) {
			m_VertexIndexMap_PositionNormal[m_VertexIndices[i]] = m_NormalIndices[i];
		}

		/* If there are UV coordinates, attempt to load an associated texture bearing the same file name */
		if (m_TextureIndices.size() > 0) {
			if (m_VertexIndexMap_PositionTexture.find(m_VertexIndices[i]) == m_VertexIndexMap_PositionTexture.end()) {
				m_VertexIndexMap_PositionTexture[m_VertexIndices[i]] = m_TextureIndices[i];
			}
		}
	}


	/* Store everything in the same order */

	for (int i = 0; i < (int)m_VertexIndexMap_PositionNormal.size(); i++) {
		int j = m_VertexIndexMap_PositionNormal[i] * 3;
		m_Normals.push_back(normalsTemp[j]);
		m_Normals.push_back(normalsTemp[j + 1]);
		m_Normals.push_back(normalsTemp[j + 2]);
	}
	for (int i = 0; i < (int)m_VertexIndexMap_PositionTexture.size(); i++) {
		int j = m_VertexIndexMap_PositionTexture[i] * 2;
		m_TextureCoordinates.push_back(texCoordTemp[j]);
		m_TextureCoordinates.push_back(texCoordTemp[j + 1]);
	}

	auto end = std::chrono::high_resolution_clock::now();

	std::chrono::duration<float> duration = end - start;
	std::cout << "Time to read file " << m_Filepath << ": " << duration.count() << " s" << std::endl;
	//} else {
	//	std::cout << "Unable to read file " << m_Filepath << std::endl;
	//}
}


/*
std::vector<std::string> split(const std::string& s, char delimiter) {
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter)) {
		tokens.push_back(token);
	}
	return tokens;
}
*/



/*


void Mesh::SetupMesh() {
	// Compile vertex attributes (position, normal, texture coordinates, etc.) into the vertex array
int numVertices = m_VertexIndexMap_PositionNormal.size();
m_Vertices.reserve(m_Positions.size() + m_Normals.size() + m_TextureCoordinates.size());

bool insertNormalsFlag = m_Normals.size() > 0;
bool insertTextureCoordsFlag = m_TextureCoordinates.size() > 0;
for (int i = 0; i < numVertices; i++) {
	m_Vertices.insert(m_Vertices.end(), m_Positions.begin() + (i * m_Dimensions), m_Positions.begin() + (i * m_Dimensions) + m_Dimensions);
	int j = i; //Nomral and texture arrays are now properly sorted so maps no longer necessary???
	if (insertNormalsFlag) {
		//int j = m_VertexIndexMap_PositionNormal[i];
		glm::vec3 normal = glm::normalize(glm::vec3(m_Normals[j * 3], m_Normals[j * 3 + 1], m_Normals[j * 3 + 2]));

		m_Vertices.push_back(normal.x);
		m_Vertices.push_back(normal.y);
		m_Vertices.push_back(normal.z);
	}

	if (insertTextureCoordsFlag) {
		//int j = m_VertexIndexMap_PositionTexture[i];
		m_Vertices.push_back(m_TextureCoordinates[j * 2]);
		m_Vertices.push_back(m_TextureCoordinates[j * 2 + 1]);
	}
}

// Setup vertex array object
m_VAO = std::make_unique<VertexArray>();
m_VertexBuffer = std::make_unique<VertexBuffer>(&m_Vertices[0],
	numVertices * (m_Dimensions
		+ 3 * insertNormalsFlag
		+ 2 * insertTextureCoordsFlag) * sizeof(float));
m_IndexBuffer = std::make_unique<IndexBuffer>(&m_VertexIndices[0], m_VertexIndices.size());

// Specify the layout of the data in each vertex (position, normal, uv-coords
VertexBufferLayout layout;
layout.Push<float>(m_Dimensions);
if (insertNormalsFlag)
layout.Push<float>(3);
if (insertTextureCoordsFlag)
layout.Push<float>(2);

// Check if we need to do instanced rendering
if (m_NumInstances > 1) {
	for (unsigned int i = 0; i < m_NumInstances; i++) {
		float v1 = (rand() % 10 + 1) * 10.1f;
		float v2 = (rand() % 10 + 1) * 10.1f;
		float v3 = (rand() % 10 + 1) * 10.1f;
		m_InstanceModelMatrices.push_back(glm::mat4(1.0f));//glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)));//(v1, v2, v3)));
	}

	m_InstanceBuffer = std::make_unique<VertexBuffer>
		(&m_InstanceModelMatrices[0], m_NumInstances * 16 * sizeof(float)); //4 vec4 columns in 4 x 4 matrix

	// Specify the layout of the data in each vertex (model matrix)
	layout.Push<glm::mat4>(1);
}

m_VAO->AddBuffer(*m_VertexBuffer, layout);

}

*/