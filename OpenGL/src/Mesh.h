#pragma once
#include <vector>

#include "Renderer.h"
#include "Shader.h"
#include "Texture.h"

#include "glm/glm.hpp"

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
	Mesh(unsigned int numInstances) : m_NumInstances(numInstances) {}
	Mesh(const std::string& filepath, unsigned int numInstances = 1);
	~Mesh();

	void Draw(const Shader& shader);

	/* Factory functions */
	
	enum SphereDivisions { res18 = 8, res16 = 16, res32 = 32, res64 = 64, res128 = 128, res256 = 256 };
	static Mesh* Sphere(SphereDivisions sphereDivisions, unsigned int numInstances) {
		Mesh* sphere = new Mesh(numInstances);
		const float PI = 3.14159265358979323846f  /* pi */;
		float sd = (float)sphereDivisions;
		int numRings = (sd / 2) - 1; //Includes equator, excludes top and bottom points
		float radRingDivs = (2.0f * PI) / sd;
		float radRingDivStart = (PI / 2) - radRingDivs; //We start at the top inner most ring and work down (but we just need values from the left half of the sphere)

		/* Top point of sphere */
		sphere->m_Positions.insert(sphere->m_Positions.end(), { 0.0f, 1.0f, 0.0f });

		for (int rIdx = 0; rIdx < numRings; rIdx++) {
			float heightRad = radRingDivStart - (radRingDivs * (float)rIdx);
			float ringScale = cos(heightRad);
			for (int i = 0; i < sd; i++) {
				float rad = (i / sd) * 2 * PI;
				sphere->m_Positions.insert(sphere->m_Positions.end(), { ringScale * cos(rad), sin(heightRad), ringScale * sin(rad) });
			}
		}

		/* Bottom point of sphere */
		sphere->m_Positions.insert(sphere->m_Positions.end(), { 0.0f, -1.0f, 0.0f });

		/* Indices for sphere top */
		for (unsigned int sIdx = 0; sIdx < sphereDivisions + 1; sIdx++) {
			unsigned int thirdIndex = sIdx + 2;
			sphere->m_PositionIndices.insert(sphere->m_PositionIndices.end(), { 0, sIdx + 1, (thirdIndex > sphereDivisions ? 1 : thirdIndex) });
		}
		
		/* Indices for inner sphere rings */
		for (int rIdx = 1; rIdx < numRings; rIdx++) {
			unsigned int currentBaseIndex = rIdx * sphereDivisions;
			unsigned int currentPeakIndex = currentBaseIndex + sphereDivisions;
			for (unsigned int sIdx = 1; sIdx < sphereDivisions + 1; sIdx++) {
				unsigned int bIdx = sIdx + ((rIdx - 1) * sphereDivisions);
				
				unsigned int lowIndex = bIdx + 1;
				unsigned int highIndex = bIdx + sphereDivisions + 1;
				unsigned int sharedIndex = (highIndex > currentPeakIndex ? currentBaseIndex + 1 : highIndex);

				if (lowIndex > currentBaseIndex) { lowIndex = (currentBaseIndex - sphereDivisions) + 1; }

				/* Bottom triangle */
				sphere->m_PositionIndices.insert(sphere->m_PositionIndices.end(), 
					{ bIdx, highIndex - 1, sharedIndex });

				/* Top triangle */
				sphere->m_PositionIndices.insert(sphere->m_PositionIndices.end(), 
					{ bIdx, sharedIndex, lowIndex });

			}
		}

		/* Indices for sphere bottom */
		unsigned int finalRingStartIndex = sphereDivisions * (numRings - 1);
		unsigned int finalIndex = (sphereDivisions * numRings) + 1;
		for (unsigned int sIdx = finalRingStartIndex; sIdx < finalIndex - 1; sIdx++) {
			unsigned int thirdIndex = sIdx + 2;
			sphere->m_PositionIndices.insert(sphere->m_PositionIndices.end(), 
				{ sIdx + 1, (thirdIndex > finalIndex - 1 ? finalRingStartIndex + 1 : thirdIndex), finalIndex });
		}

		/* Setup normals */
		sphere->m_Dimensions = 3;
		unsigned int numVertices = sphere->m_Positions.size() / 3;
		for (unsigned int i = 0; i < numVertices; i++) {
			sphere->m_VertexIndexMap_PositionNormal[i] = i;
			int pIdx = (i * 3);
			glm::vec3 norm = glm::normalize(glm::vec3(sphere->m_Positions[pIdx], sphere->m_Positions[pIdx + 1], sphere->m_Positions[pIdx + 2]));
			sphere->m_Normals.insert(sphere->m_Normals.end(), { norm.x, norm.y, norm.z });
			sphere->m_NormalIndices.push_back(i);
		}


		sphere->SetupMesh();

		return sphere;
	}
private:
	/*  Mesh Data  */
	unsigned int m_Dimensions = 0;
	std::vector<float> m_Positions;
	std::vector<float> m_Normals;
	std::vector<float> m_TextureCoordinates;
	std::vector<float> m_Vertices;
	std::unordered_map<int, int> m_VertexIndexMap_PositionNormal;
	std::unordered_map<int, int> m_VertexIndexMap_PositionTexture;

	std::vector<unsigned int> m_PositionIndices;
	std::vector<unsigned int> m_TextureIndices;
	std::vector<unsigned int> m_NormalIndices;

	std::vector<unsigned int> m_Indices;
	std::vector<MeshTexture> m_Textures;

	/* Texture data */
	std::unique_ptr<Texture> m_Texture;

	/*  Render data  */
	unsigned int m_NumInstances;
	std::unique_ptr<VertexArray> m_VAO;
	std::unique_ptr<VertexBuffer> m_VertexBuffer;
	std::unique_ptr<IndexBuffer> m_IndexBuffer;
	//std::unique_ptr<Shader> m_Shader;

	

	/*  Functions */
	void ParseMeshFile(const std::string& filepath);
	void SetupMesh();
};

