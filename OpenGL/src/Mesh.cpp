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
	//SetupMesh();	
	SetupMesh2();
}

Mesh::~Mesh() {
	//TODO: Delete stuff??
	//GLCall(glDeleteTextures(1, &m_RendererID));
}


void Mesh::Draw(const Shader& shader) {

	if (m_NumInstances > 1) {
		shader.Bind();
		m_VAO->Bind();
		/*
		
		m_IndexBuffer->Bind();

		GLCall(glDrawElementsInstanced(GL_TRIANGLES, m_IndexBuffer->GetCount(), GL_UNSIGNED_INT, 0, m_NumInstances));
		*/
	} else {
		Renderer renderer;
		renderer.Draw(*m_VAO, *m_IndexBuffer, shader);
	}
}

/* Uses the structure of arrays approach (one array for each attribute type) */
void Mesh::SetupMesh2() {
	/* Setup vertex array object */
	m_VAO = std::make_unique<VertexArray>();

	/* Create the buffers for the vertex atttributes */
	glGenBuffers(ARRAY_SIZE(m_Buffers), m_Buffers);

	/* Reserve space in the vectors for the vertex attributes and indices */
	// Generate and populate the buffers with vertex attributes and the indices
	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[POS_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(m_Positions[0]) * m_Positions.size(), &m_Positions[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(POSITION_LOCATION);
	glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[TEXCOORD_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(m_TextureCoordinates[0]) * m_TextureCoordinates.size(), &m_TextureCoordinates[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(TEXTURE_LOCATION);
	glVertexAttribPointer(TEXTURE_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[NORMAL_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(m_Normals[0]) * m_Normals.size(), &m_Normals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(NORMAL_LOCATION);
	glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[INDEX_BUFFER]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_VertexIndices[0]) * m_VertexIndices.size(), &m_VertexIndices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[WVP_MAT_VB]);
	for (unsigned int i = 0; i < 4; i++) {
		glEnableVertexAttribArray(WVP_LOCATION + i);
		glVertexAttribPointer(WVP_LOCATION + i, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4f),
			(const GLvoid*)(sizeof(GLfloat) * i * 4));
		glVertexAttribDivisor(WVP_LOCATION + i, 1);
	}

	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[WORLD_MAT_VB]);
		for (unsigned int i = 0; i < 4; i++) {
		glEnableVertexAttribArray(WORLD_LOCATION + i);
		glVertexAttribPointer(WORLD_LOCATION + i, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4f),
			(const GLvoid*)(sizeof(GLfloat) * i * 4));
		glVertexAttribDivisor(WORLD_LOCATION + i, 1);
	}
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);

	glBindVertexArray(0);
	/* Make sure the VAO is not changed from outside code */
	m_VAO->Unbind();
}

void Mesh::SetupMesh() {
	/* Compile vertex attributes (position, normal, texture coordinates, etc.) into the vertex array */
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
	
	/* Setup vertex array object */
	m_VAO = std::make_unique<VertexArray>();
	m_VertexBuffer = std::make_unique<VertexBuffer>(&m_Vertices[0],
		numVertices * (m_Dimensions
			+ 3 * insertNormalsFlag
			+ 2 * insertTextureCoordsFlag) * sizeof(float));
	m_IndexBuffer = std::make_unique<IndexBuffer>(&m_VertexIndices[0], m_VertexIndices.size());

	/* Specify the layout of the data in each vertex (position, normal, uv-coords*/
	VertexBufferLayout layout;
	layout.Push<float>(m_Dimensions);
	if (insertNormalsFlag)
		layout.Push<float>(3);
	if (insertTextureCoordsFlag)
		layout.Push<float>(2);
	
	/* Check if we need to do instanced rendering */
	if (m_NumInstances > 1) { 
		for (unsigned int i = 0; i < m_NumInstances; i++) {
			float v1 = (rand() % 10 + 1) * 10.1f;
			float v2 = (rand() % 10 + 1) * 10.1f;
			float v3 = (rand() % 10 + 1) * 10.1f;
			m_InstanceModelMatrices.push_back(glm::mat4(1.0f));//glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)));//(v1, v2, v3)));
		}

		m_InstanceBuffer = std::make_unique<VertexBuffer>
			(&m_InstanceModelMatrices[0], m_NumInstances * 16 * sizeof(float)); //4 vec4 columns in 4 x 4 matrix
		
		/* Specify the layout of the data in each vertex (model matrix)*/
		layout.Push<glm::mat4>(1);
	}

	m_VAO->AddBuffer(*m_VertexBuffer, layout);

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
	for (int i = 0; i < m_VertexIndices.size(); i++) {
		if (m_VertexIndexMap_PositionNormal.find(m_VertexIndices[i]) == m_VertexIndexMap_PositionNormal.end()) {
			m_VertexIndexMap_PositionNormal[m_VertexIndices[i]] = m_NormalIndices[i];
		}
	}

	/* If there are UV coordinates, attempt to load an associated texture bearing the same file name */
	if (m_TextureIndices.size() > 0) {
		/* Map position and normal indices based on the face data */
		for (int i = 0; i < m_VertexIndices.size(); i++) {
			if (m_VertexIndexMap_PositionTexture.find(m_VertexIndices[i]) == m_VertexIndexMap_PositionTexture.end()) {
				m_VertexIndexMap_PositionTexture[m_VertexIndices[i]] = m_TextureIndices[i];
			}
		}
	}


	/* Store the normals and texture coordinates in the same order as the positions */
	for (int i = 0; i < m_VertexIndexMap_PositionNormal.size(); i++) { 
		int j = m_VertexIndexMap_PositionNormal[i];
		m_Normals.push_back(normalsTemp[j]); 
		m_Normals.push_back(normalsTemp[j + 1]);
		m_Normals.push_back(normalsTemp[j + 2]);
	}
	for (int i = 0; i < m_VertexIndexMap_PositionTexture.size(); i++) {
		int j = m_VertexIndexMap_PositionNormal[i];
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