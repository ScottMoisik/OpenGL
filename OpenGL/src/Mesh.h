#pragma once
#include <vector>

#include "Renderer.h"
#include "Shader.h"
#include "Texture.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define INDEX_BUFFER 0 
#define POS_VB 1
#define NORMAL_VB 2
#define TEXCOORD_VB 3 
#define WORLD_MAT_VB 4
#define WVP_MAT_VB 5

#define POSITION_LOCATION 0
#define NORMAL_LOCATION 1
#define TEXTURE_LOCATION 2
#define WORLD_LOCATION 3
#define MVP_LOCATION 7


#define ARRAY_SIZE(x) (sizeof(x)/sizeof(*x))

	/* Structure data */
	struct Face {
		glm::vec3 p0, p1, p2, normal;
		int i0, i1, i2;
		float offset;
		std::vector<Face*> neighbours;
		std::vector<glm::vec3> verts;
	};

	class Mesh {
	public:
		std::string m_Filepath;
		std::string m_MaterialFilepath;

		/* Instance data */
		unsigned int m_NumInstances;
		std::vector<glm::mat4> m_InstanceMVPMatrices;
		std::vector<glm::mat4> m_InstanceModelMatrices;

		

		/*  Functions  */
		Mesh(unsigned int numInstances) : m_NumInstances(numInstances) {}
		Mesh(const std::string& filepath, unsigned int numInstances = 1);
		~Mesh();
		void Update(float deltaTime, float scale, glm::vec3 trans, float angularVel, glm::vec3 rotAxis);
		void Draw(const Shader& shader);
		void SetColor(float r, float g, float b, float a) { m_Color.x = r; m_Color.y = g; m_Color.z = b; m_Color.w = a; }
		glm::vec4 GetColor() { return m_Color; }
		Face& GetFace(int faceIndex) { return m_Faces[faceIndex]; }
		int GetNumFaces() { return m_Faces.size(); }
		int GetNumVertices() { return (int) (m_Positions.size() / 3); }
		std::vector<float>& GetPositions() { return m_Positions; }
		std::vector<unsigned int>& GetIndices() { return m_VertexIndices; }

		/* Factory functions */
		static Mesh* Plane(unsigned int numInstances) {
			Mesh* plane = new Mesh(numInstances);

			plane->m_Positions.insert(plane->m_Positions.end(), {
				-0.5f,  0.0f,  0.5f, //upper left corner
				-0.5f,  0.0f, -0.5f, //lower left corner
				 0.5f,  0.0f, -0.5f, //lower right corner
				 0.5f,  0.0f,  0.5f  //upper rigth corner 
				});

			plane->m_Normals.insert(plane->m_Normals.end(), {
				 0.0f,  1.0f,  0.0f,
				 0.0f,  1.0f,  0.0f,
				 0.0f,  1.0f,  0.0f,
				 0.0f,  1.0f,  0.0f
				});

			plane->m_TextureCoordinates.insert(plane->m_TextureCoordinates.end(), {
				 0.0f,  1.0f, //lower left corner
				 0.0f,  0.0f, //lower right corner
				 1.0f,  0.0f, //upper right corner
				 1.0f,  1.0f  //upper left corner
				});

			plane->m_VertexIndices.insert(plane->m_VertexIndices.end(), {
				0, 1, 2,
				2, 3, 0
				});

			plane->SetupMesh();

			return plane;

		}

		//Create triangle strip based arrow
		static Mesh* Arrow(unsigned int numInstances) {
			using namespace glm;
			Mesh* arrow = new Mesh(numInstances);
			const float PI = 3.14159265358979323846f;  // pi;
			const float PI6 = PI / 6.0f;
			const float PI3 = PI / 3.0f;
			const float PI23 = 3.0f * (PI / 2.0f);
			const float arrowHeadTip = 1.0f;
			const float arrowHeadBase = 0.8f;
			const float headScale = 0.5;
			const float shaftScale = 0.5;

			//Arrow head
			arrow->m_Positions.insert(arrow->m_Positions.begin(), {
				arrowHeadTip,	0.0f,						0.0f,						//arrow tip
				arrowHeadBase,	headScale*sin(PI6),			headScale*cos(PI6),			//arrow head base, upper right
				arrowHeadBase,	headScale*sin(PI6 + PI3),	headScale*cos(PI6 + PI3),	//arrow head base, upper left
				arrowHeadBase,	headScale*sin(PI23),		headScale*cos(PI23)			//arrow head base, bottom
				});

			//Arrow shaft lambda
			auto buildArrowLambda = [&arrow, &arrowHeadBase, &shaftScale](float theta) {
				arrow->m_Positions.insert(arrow->m_Positions.begin(), {
					arrowHeadBase,	shaftScale*sin(theta),		shaftScale*cos(theta),		//arrow shaft tip
					0.0f,			shaftScale*sin(theta),		shaftScale*cos(theta),		//arrow shaft base
					});
			};

			buildArrowLambda(PI6);
			buildArrowLambda(PI6 + PI3);
			buildArrowLambda(PI23);

			arrow->m_VertexIndices.insert(arrow->m_VertexIndices.end(), {
				//Begin arrow head
				0, 1, 2,	0, 2, 3,	0, 3, 1, 
				//Begin arrow shaft
				4, 5, 6,	6, 7, 4,
				7, 6, 9,	9, 8, 7,
				8, 9, 10,	10, 11, 8});

			arrow->SetupMesh();

			return arrow;
		}

		//Winding order checking algorithm: http://www.dillonbhuff.com/?p=30
		static Mesh* Tetrahedron(unsigned int numInstances, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3) {
			Mesh* tet = new Mesh(numInstances);

			/* Use a lambda to shorten the code for inserting positions */
			auto buildTetLambda = [&tet](glm::vec3 a, glm::vec3 b, glm::vec3 c) {
				tet->m_Positions.insert(tet->m_Positions.end(), { a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z });
				glm::vec3 n0 = glm::normalize(glm::cross(b - a, c - a));
				tet->m_Normals.insert(tet->m_Normals.end(), { n0.x, n0.y, n0.z, n0.x, n0.y, n0.z, n0.x, n0.y, n0.z });
			};

			buildTetLambda(v0, v1, v3);
			buildTetLambda(v1, v2, v3);
			buildTetLambda(v0, v3, v2);
			buildTetLambda(v0, v2, v1);

			tet->m_TextureCoordinates.insert(tet->m_TextureCoordinates.end(), {
				0.0, 0.0, 1.0, 0.0, 0.5, 0.5,
				1.0, 0.0, 1.0, 1.0, 0.5, 0.5,
				1.0, 1.0, 0.0, 1.0, 0.5, 0.5,
				0.0, 1.0, 0.0, 0.0, 0.5, 0.5 });

			tet->m_VertexIndices.insert(tet->m_VertexIndices.end(), {
				0, 1, 2,
				3, 4, 5,
				6, 7, 8,
				9, 10, 11 });

			tet->m_Normals.clear();
			for (int i = 0; i < (int)tet->m_VertexIndices.size() / 3; i++) {
				int i0 = tet->m_VertexIndices[i * 3];
				int i1 = tet->m_VertexIndices[i * 3 + 1];
				int i2 = tet->m_VertexIndices[i * 3 + 2];

				glm::vec3 p0 = glm::vec3(tet->m_Positions[i0 * 3], tet->m_Positions[i0 * 3 + 1], tet->m_Positions[i0 * 3 + 2]);
				glm::vec3 p1 = glm::vec3(tet->m_Positions[i1 * 3], tet->m_Positions[i1 * 3 + 1], tet->m_Positions[i1 * 3 + 2]);
				glm::vec3 p2 = glm::vec3(tet->m_Positions[i2 * 3], tet->m_Positions[i2 * 3 + 1], tet->m_Positions[i2 * 3 + 2]);

				glm::vec3 n0 = glm::normalize(glm::cross(p2 - p0, p1 - p0));
				tet->m_Normals.insert(tet->m_Normals.end(), { n0.x, n0.y, n0.z, n0.x, n0.y, n0.z, n0.x, n0.y, n0.z });
			};

			tet->SetupMesh();

			return tet;
		}

		static Mesh* Cube(unsigned int numInstances) {
			Mesh* cube = new Mesh(numInstances);
			const float PI = 3.14159265358979323846f  /* pi */;
			glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

			/* Use a lambda to shorten the code for inserting normals */
			auto buildCubeLambda = [&cube, &up, &PI](float x, float y, float z) {
				glm::vec3 normal = glm::vec3(x, y, z);
				glm::vec3 shift = normal;
				shift *= 0.5;

				glm::mat4 R = glm::mat4(1.0f);
				glm::vec3 axis = glm::cross(normal, up);
				/* Make sure that the axis is not close to zero (if normal and up are in same direction) */
				if (glm::length(axis) > 0.1) {
					R = glm::rotate(glm::mat4(1.0f), PI / 2, axis);
				}
				glm::mat4 T = glm::translate(glm::mat4(1.0f), shift);

				float verts[] = { -0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, 0.5f, 0.0f, 0.5f, 0.5f, 0.0f, -0.5f };
				for (int i = 0; i < 4; i++) {
					glm::vec4 vert = T * R * glm::vec4(verts[i * 3], verts[i * 3 + 1], verts[i * 3 + 2], 1.0f);
					cube->m_Positions.insert(cube->m_Positions.end(), { vert.x, vert.y, vert.z });
				}

				cube->m_Normals.insert(cube->m_Normals.end(), { x, y, z, x, y, z, x, y, z, x, y, z });
			};

			buildCubeLambda(-1.0f, 0.0f, 0.0f); //left
			buildCubeLambda(0.0f, 0.0f, 1.0f); //front
			buildCubeLambda(0.0f, -1.0f, 0.0f); //bottom
			buildCubeLambda(0.0f, 0.0f, -1.0f); //behind
			buildCubeLambda(1.0f, 0.0f, 0.0f); //right
			buildCubeLambda(0.0f, 1.0f, 0.0f); //top

			/* Maps the texture to a unfolded cube set within and flush against a rectangular image */
			float coords[] = { 0.0f, 2.0f / 3.0f, 1.0f / 4.0f, 1.0f / 3.0f, 1.0f / 4.0f, 2.0f / 3.0f, 1.0f / 4.0f, 3.0f / 3.0f, 2.0f / 4.0f, 2.0f / 3.0f, 3.0f / 4.0f, 2.0f / 3.0f };
			auto insertTextureCoordsLambda = [&cube](float left, float top) { cube->m_TextureCoordinates.insert(cube->m_TextureCoordinates.end(), { left, top, left, top - 1.0f / 3.0f, left + 1.0f / 4.0f, top - 1.0f / 3.0f, left + 1.0f / 4.0f, top }); };
			for (int i = 0; i < 6; i++) {
				insertTextureCoordsLambda(coords[i * 2], coords[i * 2 + 1]);
			}

			auto insertVertexIndicesLambda = [&cube](unsigned int i) { cube->m_VertexIndices.insert(cube->m_VertexIndices.end(), { i, i + 1, i + 2, i + 2, i + 3, i }); };
			for (int i = 0; i < 6; i++) {
				insertVertexIndicesLambda(4 * i);
			}

			cube->SetupMesh();

			return cube;

		}


		enum SphereDivisions { res18 = 8, res16 = 16, res32 = 32, res64 = 64, res128 = 128, res256 = 256 };
		static Mesh* Sphere(SphereDivisions sphereDivisions, unsigned int numInstances) {
			Mesh* sphere = new Mesh(numInstances);
			const float PI = 3.14159265358979323846f  /* pi */;
			float sd = (float)sphereDivisions;
			unsigned int sphereDiv = (unsigned int)sphereDivisions;
			int numRings = (sphereDivisions / 2) - 1; //Includes equator, excludes top and bottom points
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
			for (unsigned int sIdx = 0; sIdx < sphereDiv + 1; sIdx++) {
				unsigned int thirdIndex = sIdx + 2;
				sphere->m_VertexIndices.insert(sphere->m_VertexIndices.end(), { 0, sIdx + 1, (thirdIndex > sphereDiv ? 1 : thirdIndex) });
			}

			/* Indices for inner sphere rings */
			for (int rIdx = 1; rIdx < numRings; rIdx++) {
				unsigned int currentBaseIndex = rIdx * sphereDiv;
				unsigned int currentPeakIndex = currentBaseIndex + sphereDiv;
				for (unsigned int sIdx = 1; sIdx < sphereDiv + 1; sIdx++) {
					unsigned int bIdx = sIdx + ((rIdx - 1) * sphereDiv);

					unsigned int lowIndex = bIdx + 1;
					unsigned int highIndex = bIdx + sphereDiv + 1;
					unsigned int sharedIndex = (highIndex > currentPeakIndex ? currentBaseIndex + 1 : highIndex);

					if (lowIndex > currentBaseIndex) { lowIndex = (currentBaseIndex - sphereDiv) + 1; }

					/* Bottom triangle */
					sphere->m_VertexIndices.insert(sphere->m_VertexIndices.end(),
						{ bIdx, highIndex - 1, sharedIndex });

					/* Top triangle */
					sphere->m_VertexIndices.insert(sphere->m_VertexIndices.end(),
						{ bIdx, sharedIndex, lowIndex });

				}
			}

			/* Indices for sphere bottom */
			unsigned int finalRingStartIndex = sphereDivisions * (numRings - 1);
			unsigned int finalIndex = (sphereDivisions * numRings) + 1;
			for (unsigned int sIdx = finalRingStartIndex; sIdx < finalIndex - 1; sIdx++) {
				unsigned int thirdIndex = sIdx + 2;
				sphere->m_VertexIndices.insert(sphere->m_VertexIndices.end(),
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

		std::vector<Face> m_Faces;

		//  Mesh Data 
		unsigned int m_Dimensions = 0;
		glm::vec4 m_Color;
		std::vector<float> m_Positions;
		std::vector<float> m_Normals;
		std::vector<float> m_Tangents;
		std::vector<float> m_Bitangents;
		std::vector<float> m_TextureCoordinates;
		std::vector<float> m_Vertices;
		std::unordered_map<int, int> m_VertexIndexMap_PositionNormal;
		std::unordered_map<int, int> m_VertexIndexMap_PositionTexture;

		std::vector<unsigned int> m_PositionIndices;
		std::vector<unsigned int> m_TextureIndices;
		std::vector<unsigned int> m_NormalIndices;

		std::vector<unsigned int> m_VertexIndices;

		/* Instance data */
		std::unique_ptr<VertexArray> m_InstanceVAO;
		std::unique_ptr<VertexBuffer> m_InstanceBuffer;

		/* Texture data */
		std::unique_ptr<Texture> m_Texture;

		/*  Render data  */
		std::unique_ptr<VertexArray> m_VAO;
		std::unique_ptr<VertexBuffer> m_VertexBuffer;
		std::unique_ptr<IndexBuffer> m_IndexBuffer;
		//std::unique_ptr<Shader> m_Shader;

		/* Vertex buffer data */
		unsigned int m_Buffers[6]; //indices, positions, normals, texture coords
		unsigned int m_NumBuffers = 6; //Size of m_Buffers

		/*  Functions */
		void ParseMeshFile(const std::string& filepath);
		void SetupMesh();
		void CreateFaces();
		void FixWinding();
	};
