#include "Mesh.h"

#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "VertexBufferLayout.h"

Mesh::Mesh(const std::string& filepath) : m_Filepath(filepath) {
	ParseMeshFile(filepath);

	m_VAO = std::make_unique<VertexArray>();
	m_VertexBuffer = std::make_unique<VertexBuffer>(&m_Positions[0], m_Positions.size() * m_Dimensions * sizeof(float));
	m_IndexBuffer = std::make_unique<IndexBuffer>(&m_PositionIndices[0], m_PositionIndices.size());

	GLCall(glBufferData(GL_ARRAY_BUFFER, m_Positions.size() * m_Dimensions * sizeof(float), &m_Positions[0], GL_STATIC_DRAW));

	VertexBufferLayout layout;
	layout.Push<float>(3);
	m_VAO->AddBuffer(*m_VertexBuffer, layout);
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

							if (secondSlash - firstSlash != 1) {
								std::string textureIndex = token.substr(firstSlash + 1, secondSlash - firstSlash);
								m_TextureIndices.push_back(std::stoi(textureIndex));
							} else {
								std::string vertexIndex = token.substr(0, firstSlash + 1);
								m_PositionIndices.push_back(std::stoi(vertexIndex));

								std::string normalIndex = token.substr(secondSlash + 1);
								m_NormalIndices.push_back(std::stoi(normalIndex));
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