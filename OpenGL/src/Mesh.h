#pragma once
#include <vector>

#include "Renderer.h"
#include "Shader.h"

struct MeshVertex {
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
};

struct MeshTexture {
	unsigned int id;
	std::string type;
};



class Mesh {
public:
	std::string m_Filepath;
	std::string m_MaterialFilepath;

	/*  Functions  */
	Mesh(const std::string& filepath);
	~Mesh();
	void Draw(const Shader& shader);
private:
	/*  Mesh Data  */
	unsigned int m_Dimensions = 0;
	std::vector<float> m_Positions;
	std::vector<float> m_Normals;
	//std::vector<Vertex> m_Vertices;
	std::vector<unsigned int> m_PositionIndices;
	std::vector<unsigned int> m_TextureIndices;
	std::vector<unsigned int> m_NormalIndices;

	std::vector<unsigned int> m_Indices;
	std::vector<MeshTexture> m_Textures;

	/*  Render data  */
	std::unique_ptr<VertexArray> m_VAO;
	std::unique_ptr<VertexBuffer> m_VertexBuffer;
	std::unique_ptr<IndexBuffer> m_IndexBuffer;
	//std::unique_ptr<Shader> m_Shader;

	

	/*  Functions */
	void ParseMeshFile(const std::string& filepath);
};

