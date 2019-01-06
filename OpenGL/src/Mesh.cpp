#include "Mesh.h"

#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath>

#include "VertexBufferLayout.h"

Mesh::Mesh(const std::string& filepath) : m_Filepath(filepath) {
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
	for (unsigned int i = 0; i < textures.size(); i++)
	{
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

	// draw mesh
	Renderer renderer;
	renderer.Draw(*m_VAO, *m_IndexBuffer, shader);
}






enum AttributeType { NONE, POSITION, VERTEX_NORMAL, FACE };

class VertexAttribute {
public:
	VertexAttribute(const std::string& type) {
		if (type.find("v") != std::string::npos && type.size() == 1) {
			at = POSITION; //Positions are given as "v" in Wavefront .OBJ 
		} else if (type.find("vn") != std::string::npos) {
			at = VERTEX_NORMAL;
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

				while (std::getline(tokenStream, token, ' ')) {
					//std::cout << " " << token << " ";
					try {
						if (at.IsType(AttributeType::POSITION)) {
							m_Positions.push_back(std::stof(token));
						} else if (at.IsType(AttributeType::VERTEX_NORMAL)) {
							m_Normals.push_back(std::stof(token));
						} else if (at.IsType(AttributeType::FACE)) {
							int firstSlash = token.find("/");
							int secondSlash = token.find("/", firstSlash + 1);

							/* Store indices, subtracting 1 because Wavefront .OBJ indices start at 1*/
							if (secondSlash - firstSlash != 1) {
								std::string textureIndex = token.substr(firstSlash + 1, secondSlash - firstSlash);
								m_TextureIndices.push_back(std::stoi(textureIndex) - 1);
							} else {
								std::string vertexIndex = token.substr(0, firstSlash + 1);
								m_PositionIndices.push_back(std::stoi(vertexIndex) - 1);

								std::string normalIndex = token.substr(secondSlash + 1);
								m_NormalIndices.push_back(std::stoi(normalIndex) - 1);
							}
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

	//std::vector<float>::iterator posIter;
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
			m_Vertices.insert(m_Vertices.end(), m_TextureCoordinates.begin() + (i * m_Dimensions), m_TextureCoordinates.begin() + (i * m_Dimensions) + m_Dimensions);
		}
	}


	m_VAO = std::make_unique<VertexArray>();
	m_VertexBuffer = std::make_unique<VertexBuffer>(&m_Vertices[0],
		numVertices * (m_Dimensions + 3 * insertNormalsFlag + 2 * insertTextureCoordsFlag) * sizeof(float));
	m_IndexBuffer = std::make_unique<IndexBuffer>(&m_PositionIndices[0], m_PositionIndices.size());

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