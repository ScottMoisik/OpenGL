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


void Mesh::Draw(const Shader& shader) {
	/*
	unsigned int diffuseNr = 1;
	unsigned int specularNr = 1;
	for (unsigned int i = 0; i < textures.size(); i++) {
		glActiveTexture(GL_TEXTURE0 + i); // activate proper texture unit before binding
		// retrieve texture number (the N in diffuse_textureN)
		string number;
		string name = textures[i].type;
		if (name == "texture_diffuse")
			number = std::to_string(diffuseNr++);
		else if (name == "texture_specular")
			number = std::to_string(specularNr++);

		shader.setFloat(("material." + name + number).c_str(), i);
		glBindTexture(GL_TEXTURE_2D, textures[i].id);
	}
	glActiveTexture(GL_TEXTURE0);
	*/
	

	if (m_NumInstances > 1) {
		shader.Bind();
		m_VAO->Bind();
		m_IndexBuffer->Bind();

		m_InstanceVAO->Bind();
		m_InstanceBuffer->Bind();
		

		GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * m_NumInstances, &m_InstanceModelMatrices[0], GL_DYNAMIC_DRAW));
		
		GLCall(glDrawElementsInstanced(GL_TRIANGLES, m_IndexBuffer->GetCount(), GL_UNSIGNED_INT, 0, m_NumInstances));

	} else {
		Renderer renderer;
		renderer.Draw(*m_VAO, *m_IndexBuffer, shader);
	}
	
	
	
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
							m_Normals.push_back(std::stof(token));
						} else if (at.IsType(AttributeType::TEXTURE_UV_COORD)) {
							m_TextureCoordinates.push_back(std::stof(token));
						} else if (at.IsType(AttributeType::FACE)) {
							/* Check if this is the 4th vertex, and, if so, copy the first and third previous indices to complete the triangle */
							if (vertexCount > 3) {
								/* TO convert a quad to two triangles, we first need to copy the previous indices */
								m_PositionIndices.push_back(m_PositionIndices.back());
								m_NormalIndices.push_back(m_NormalIndices.back());
								if (m_TextureIndices.size() > 0) { m_TextureIndices.push_back(m_TextureIndices.back()); }
							}

							int firstSlash = token.find("/");
							int secondSlash = token.find("/", firstSlash + 1);

							/* Store indices, subtracting 1 because Wavefront .OBJ indices start at 1*/
							std::string vertexIndex = token.substr(0, firstSlash);
							m_PositionIndices.push_back(std::stoi(vertexIndex) - 1);

							std::string normalIndex = token.substr(secondSlash + 1);
							m_NormalIndices.push_back(std::stoi(normalIndex) - 1);
														
							if (secondSlash - firstSlash != 1) {
								std::string textureIndex = token.substr(firstSlash + 1, firstSlash + (secondSlash - firstSlash) - 1);
								m_TextureIndices.push_back(std::stoi(textureIndex) - 1);
							}

							/* Check if this is the 4th vertex, and, if so, copy the first and third previous indices to complete the triangle */
							if (vertexCount > 3) {
								/* To convert a quad to two triangles, we then need to copy the first indices that we found in this line */
								m_PositionIndices.push_back(m_PositionIndices[m_PositionIndices.size() - 5]);
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
		for (int i = 0; i < m_PositionIndices.size(); i++) {
			if (m_VertexIndexMap_PositionNormal.find(m_PositionIndices[i]) == m_VertexIndexMap_PositionNormal.end()) {
				m_VertexIndexMap_PositionNormal[m_PositionIndices[i]] = m_NormalIndices[i];
			}
		}
		
		/* If there are UV coordinates, attempt to load an associated texture bearing the same file name */
		if (m_TextureIndices.size() > 0) {
			//std::string texturePath = filepath;
			//texturePath.replace(texturePath.size() - 3, 3, "jpg");
			//m_Texture = std::make_unique<Texture>(texturePath);
			//m_Shader->SetUniform1i("u_Texture", 0);

			/* Map position and normal indices based on the face data */
			for (int i = 0; i < m_PositionIndices.size(); i++) {
				if (m_VertexIndexMap_PositionTexture.find(m_PositionIndices[i]) == m_VertexIndexMap_PositionTexture.end()) {
					m_VertexIndexMap_PositionTexture[m_PositionIndices[i]] = m_TextureIndices[i];
				}
			}
		}

		auto end = std::chrono::high_resolution_clock::now();

		std::chrono::duration<float> duration = end - start;
		std::cout << "Time to read file " << m_Filepath << ": " << duration.count() << " s" << std::endl;
	//} else {
	//	std::cout << "Unable to read file " << m_Filepath << std::endl;
	//}
}


void Mesh::SetupMesh() {
	/* Compile vertex attributes (position, normal, texture coordinates, etc.) into the vertex array */
	int numVertices = m_VertexIndexMap_PositionNormal.size();
	m_Vertices.reserve(m_Positions.size() + m_Normals.size() + m_TextureCoordinates.size());

	bool insertNormalsFlag = m_Normals.size() > 0;
	bool insertTextureCoordsFlag = m_TextureCoordinates.size() > 0;
	for (int i = 0; i < numVertices; i++) {
		m_Vertices.insert(m_Vertices.end(), m_Positions.begin() + (i * m_Dimensions), m_Positions.begin() + (i * m_Dimensions) + m_Dimensions);

		if (insertNormalsFlag) {
			int j = m_VertexIndexMap_PositionNormal[i];
			glm::vec3 normal = glm::normalize(glm::vec3(m_Normals[j * 3], m_Normals[j * 3 + 1], m_Normals[j * 3 + 2]));

			m_Vertices.push_back(normal.x);
			m_Vertices.push_back(normal.y);
			m_Vertices.push_back(normal.z);
		}

		if (insertTextureCoordsFlag) {
			int j = m_VertexIndexMap_PositionTexture[i];
			m_Vertices.push_back(m_TextureCoordinates[j * 2]);
			m_Vertices.push_back(m_TextureCoordinates[j * 2 + 1]);
		}
	}

	/* Check if we need to do instanced rendering */
	if (m_NumInstances > 1) { 
		for (unsigned int i = 0; i < m_NumInstances; i++) {
			float v1 = (rand() % 10 + 1) * 0.1f;
			float v2 = (rand() % 10 + 1) * 0.1f;
			float v3 = (rand() % 10 + 1) * 0.1f;
			m_InstanceModelMatrices.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(v1, v2, v3)));
		}
		/* Setup vertex array object */
		m_InstanceVAO = std::make_unique<VertexArray>();
		m_InstanceBuffer = std::make_unique<VertexBuffer>
			(&m_InstanceModelMatrices[0], m_NumInstances * 4 * 4 * sizeof(float)); //4 vec4 columns in 4 x 4 matrix
		
		/* Specify the layout of the data in each vertex (model matrix)*/
		VertexBufferLayout layout;
		layout.Push<glm::mat4>(1);
		m_InstanceVAO->AddBuffer(*m_InstanceBuffer, layout);
	}

	/* Setup vertex array object */
	m_VAO = std::make_unique<VertexArray>();
	m_VertexBuffer = std::make_unique<VertexBuffer>(&m_Vertices[0], 
		numVertices * (m_Dimensions 
			+ 3 * insertNormalsFlag 
			+ 2 * insertTextureCoordsFlag) * sizeof(float));
	m_IndexBuffer = std::make_unique<IndexBuffer>(&m_PositionIndices[0], m_PositionIndices.size());
	
	/* Specify the layout of the data in each vertex (position, normal, uv-coords*/
	VertexBufferLayout layout;
	layout.Push<float>(m_Dimensions);
	if (insertNormalsFlag)
		layout.Push<float>(3);
	if (insertTextureCoordsFlag)
		layout.Push<float>(2);

	m_VAO->AddBuffer(*m_VertexBuffer, layout);
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