#include "TestMesh.h"

#include "Renderer.h"
#include "Mesh.h"

#include "imgui/imgui.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <GLFW/glfw3.h>


namespace Test {

	TestMesh::TestMesh() :
		m_Proj(glm::perspective(glm::radians(45.0f), 3.0f/4.0f, 0.1f, 100.0f)),
		m_View(glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 5.0f, 5.0f))),
		m_Translation(0.0f, 0.0f, 0.0f), m_LightPosition(3.0f, 5.0f, 0.0f) {
				
		GLint m_viewport[4];
		GLCall(glGetIntegerv(GL_VIEWPORT, m_viewport));
		m_ViewPortWidth = (float)m_viewport[2];
		m_ViewPortHeight = (float)m_viewport[3];
		m_AspectRatio = m_ViewPortWidth / m_ViewPortHeight;

		GLCall(glEnable(GL_BLEND));
		GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
		
		/* Enable depth testing */
		GLCall(glEnable(GL_DEPTH_TEST));
		
		/* TEST AREA */
		//m_Mesh = (std::unique_ptr<Mesh>)Mesh::Sphere(Mesh::res256, 2);
		//m_Mesh = std::make_unique<Mesh>("res/meshes/suzanne.obj");

		int numInstances = 4;
		//std::string shaderFilepath = "res/shaders/BasicMesh.shader";
		
		m_MeshLight = (std::unique_ptr<Mesh>)Mesh::Sphere(Mesh::SphereDivisions::res32, 1);
		m_MeshLight->SetColor(0.9f, 0.9f, 1.0f, 1.0f);

		m_MeshBox = (std::unique_ptr<Mesh>)Mesh::Cube(1);
		m_MeshBox->SetColor(0.9f, 0.9f, 1.0f, 1.0f);

		m_MeshPlane = (std::unique_ptr<Mesh>)Mesh::Plane(1);
		m_MeshPlane->SetColor(0.2f, 0.2f, 0.6f, 1.0f);
		m_PlaneTexture = std::make_unique<Texture>("res/textures/marble.jpg");

 		m_Mesh = std::make_unique<Mesh>("res/meshes/earth.obj", numInstances);
		m_Texture = std::make_unique<Texture>("res/textures/earth.jpg");

		// Normal mapping example (brickwall)
		m_TextureBrickDiffuse = std::make_unique<Texture>("res/textures/brickwall.jpg");
		m_TextureBrickNormal = std::make_unique<Texture>("res/textures/brickwall_normal.jpg");
		m_TextureBrickDepth = std::make_unique<Texture>("res/textures/brickwall_depth.jpg");
		m_NormalMappingShader = std::make_unique<Shader>("res/shaders/ParallaxMappingInstanced.shader");
		m_NormalMappingShader->Bind();
		m_NormalMappingShader->SetUniform1i("u_DiffuseMap", 1);
		m_NormalMappingShader->SetUniform1i("u_NormalMap", 2);

		/* Load shaders for the scene */
		m_BasicShader = std::make_unique<Shader>("res/shaders/Basic.shader");
		m_BasicShader->Bind();

		m_SimpleShader = std::make_unique<Shader>("res/shaders/ColorOnly.shader");
		m_SimpleShader->Bind();
		m_SimpleShader->SetUniform4f("u_ObjectColor", 0.8f, 0.8f, 1.0f, 0.7f);

		m_Shader = std::make_unique<Shader>("res/shaders/BasicMeshInstanced.shader");
		m_Shader->Bind();
		m_Shader->SetUniform3f("u_LightColor", 0.6f, 0.6f, 0.6f);
		m_Shader->SetUniform4f("u_ObjectColor", 0.8f, 0.3f, 0.8f, 1.0f);

		m_Shader->SetUniform1b("u_UseTexturing", true);
		m_Shader->Unbind();

		/* Load depth shader for shadows */
		m_DepthShader = std::make_unique<Shader>("res/shaders/SimpleDepth.shader");
		m_DepthShader->Bind();

		m_DebugDepthQuadShader = std::make_unique<Shader>("res/shaders/DebugQuad.shader");
		m_DebugDepthQuadShader->Bind();
		m_DebugDepthQuadShader->SetUniform1i("depthMap", 0);

		/* Load normal visualizing shader */
		//m_NormalVisualizingShader = std::make_unique<Shader>("res/shaders/NormalVisualization.shader");
		m_NormalVisualizingShader = std::make_unique<Shader>("res/shaders/NormalVisualizationInstanced.shader");


		
		// create depth texture
		glGenTextures(1, &depthMap);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

		// configure depth map FBO and attach depth texture as FBO's depth buffer
		glGenFramebuffers(1, &depthMapFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		
		glDrawBuffer(GL_NONE); //No color texture will be bound to this FBO
		glReadBuffer(GL_NONE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}


	TestMesh::~TestMesh() {}
	void TestMesh::OnUpdate(float deltaTime) {
		float angularVel = 3.14f / 10.0f;		
		m_Mesh->Update(deltaTime, 1.0f, glm::vec3(5.0f, 3.0f, 0.0f), angularVel, glm::vec3(0.0f, 1.0f, 0.0f));
		m_MeshPlane->Update(deltaTime, 100.0f, glm::vec3(0.0f), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		m_MeshBox->Update(deltaTime, 2.0f, glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		m_MeshLight->Update(deltaTime, 1.0f, m_LightPosition, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	}

	enum ProjectionType { ORTHO, PERSPECTIVE };
	void TestMesh::OnRender() {
		GLCall(glClearColor(0.2f, 0.2f, 0.6f, 1.0f));
		GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
		
		ProjectionType projType = ProjectionType::ORTHO;

		{
			using namespace glm;
			/* 1: Shadown rendering: Set up shadow rendering: render depth of scene from light's perspective */
			
			glm::mat4 lightProjection, lightView, lightSpaceMatrix;
			float near_plane = -10.0f, far_plane = 10.0f;
			if (projType == ORTHO) {
				float range = 50.0;
				lightProjection = glm::ortho(-range, range, -range, range, near_plane, far_plane);
			} else {
				lightProjection = glm::perspective(glm::radians(45.0f), 1.0f, near_plane, far_plane);
			}

			
			lightView = glm::lookAt(m_LightPosition, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0, 1.0, 0.0));
			lightSpaceMatrix = lightProjection * lightView;

			glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
			glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
			glClear(GL_DEPTH_BUFFER_BIT);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, depthMap);

			//glCullFace(GL_FRONT);
			m_DepthShader->Bind();
			m_DepthShader->SetUniformMat4f("u_LightSpaceMatrix", lightSpaceMatrix);
			m_MeshBox->Draw(*m_DepthShader);
			m_Mesh->Draw(*m_DepthShader);
			//glCullFace(GL_BACK); // don't forget to reset original culling face

			glBindFramebuffer(GL_FRAMEBUFFER, 0); //Return to default frame buffer
			

			/* 2: Ordinary rendering with shadows determined from depth map */
			glViewport(0, 0, m_Width, m_Height); //SCR_WIDTH, SCR_HEIGHT);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			/* Camera processing */
			m_Proj = glm::perspective(glm::radians(m_Camera->Zoom), m_Camera->m_AspectRatio, 1.0f, 1000.0f);
			m_View = m_Camera->GetViewMatrix();
			
			
			/*
			glm::mat4 biasMatrix(
				0.5, 0.0, 0.0, 0.0,
				0.0, 0.5, 0.0, 0.0,
				0.0, 0.0, 0.5, 0.0,
				0.5, 0.5, 0.5, 1.0
			);

			glm::mat4 depthBiasMVP = biasMatrix * lightSpaceMatrix;
			*/


			/* Set uniforms for the basic shader */
			mat4 model = translate(mat4(1.0f), m_LightPosition);
			mat4 MVP = m_Proj * m_View * model; //GLM is column-major memory layout so requires reverse multiplication for MVP

			// Setup textures for the light
			m_SimpleShader->Bind();
			m_SimpleShader->SetUniformMat4f("u_MVP", MVP);
			//m_MeshLight->Draw(*m_SimpleShader);

			// Set uniforms for the basic shader
			MVP = m_Proj * m_View * model; //GLM is column-major memory layout so requires reverse multiplication for MVP
			model = translate(mat4(1.0f), m_Translation);
			model = rotate(model, radians(m_Rotation), vec3(0.0f, 1.0f, 0.0f));

			//Setup more complex shader for rest of objects
			m_Shader->Bind();		
			m_Shader->SetUniform1i("u_ShadowMap", 0);
			m_Shader->SetUniform1i("u_Texture", 1);			
			
			m_Shader->SetUniformMat4f("u_Proj", m_Proj);
			m_Shader->SetUniformMat4f("u_View", m_View);
			m_Shader->SetUniformMat4f("u_LightSpaceMatrix", lightSpaceMatrix);
			m_Shader->SetUniform3f("u_ViewPos", m_Camera->Position.x, m_Camera->Position.y, m_Camera->Position.z);
			m_Shader->SetUniform3f("u_LightPosition", m_LightPosition.x, m_LightPosition.y, m_LightPosition.z);
			m_Shader->SetUniform4f("u_ObjectColor", 0.5, 0.1, 0.1, 1.0f);
			//m_Shader->SetUniform1b("u_UseTexturing", true);

			m_Texture->Bind(1);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, depthMap);
			m_MeshBox->Draw(*m_Shader);
			
			// Do draw call for mesh shader
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, depthMap);
			m_Texture->Bind(1);
			m_Mesh->Draw(*m_Shader);
			
			m_PlaneTexture->Bind(1);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, depthMap);
			m_MeshPlane->Draw(*m_Shader);
			//m_PlaneTexture->Unbind();


			
			// render normal-mapped quad
			glm::mat4 brickModel = translate(mat4(1.0f), vec3(5.0f, 5.0f, 5.0f)) * scale(mat4(1.0f), vec3(2.0f));
			//brickModel = glm::rotate(model, glm::radians((float)glfwGetTime() * 0.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0))); // rotate the quad to show normal mapping from multiple directions
			m_NormalMappingShader->Bind();
			m_NormalMappingShader->SetUniform1i("u_ShadowMap", 0);
			m_NormalMappingShader->SetUniform1i("u_DiffuseMap", 1);
			m_NormalMappingShader->SetUniform1i("u_NormalMap", 2);
			m_NormalMappingShader->SetUniform1i("u_DepthMap", 2);

			m_NormalMappingShader->SetUniform3f("u_ViewPos", m_Camera->Position.x, m_Camera->Position.y, m_Camera->Position.z);
			m_NormalMappingShader->SetUniform3f("u_LightPos", m_LightPosition.x, m_LightPosition.y, m_LightPosition.z);

			m_NormalMappingShader->SetUniformMat4f("u_Model", brickModel);
			m_NormalMappingShader->SetUniformMat4f("u_View", m_View);
			m_NormalMappingShader->SetUniformMat4f("u_Proj", m_Proj);
			m_NormalMappingShader->SetUniformMat4f("u_LightSpaceMatrix", lightSpaceMatrix);
			m_NormalMappingShader->SetUniform1f("heightScale", m_Rotation); // adjust with Q and E keys

			m_TextureBrickDiffuse->Bind(1);
			m_TextureBrickNormal->Bind(2);
			m_TextureBrickDepth->Bind(3);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, depthMap);
			m_TextureBrickDiffuse->Bind(0);
			renderNormalMappedQuad();


			if (m_NormalVisualizationFlag) { 
				/* Set uniforms for the normal visualizing shader */
				m_NormalVisualizingShader->Bind();
				m_NormalVisualizingShader->SetUniformMat4f("u_Proj", m_Proj);
				m_NormalVisualizingShader->SetUniformMat4f("u_View", m_View);
				m_NormalVisualizingShader->SetUniformMat4f("u_Model", model);

				/* Do draw call for normal visualizing shader*/
				m_Mesh->Draw(*m_NormalVisualizingShader); 
				m_NormalVisualizingShader->Unbind();
			}

			/* render depth map to quad for visual debugging */
			glViewport(0, 0, 512, 512);
			m_DebugDepthQuadShader->Bind();
			m_DebugDepthQuadShader->SetUniform1b("u_OrthographicFlag", projType == ORTHO);
			m_DebugDepthQuadShader->SetUniform1f("u_NearPlane", near_plane);
			m_DebugDepthQuadShader->SetUniform1f("u_FarPlane", far_plane);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, depthMap);
			renderQuad();

			
		}

	}


	void TestMesh::RenderScene() {

	}

	void TestMesh::OnImGuiRender() {
		//ImGui::SliderInt("shadow resolution factor", &m_ShadowResolution, 1, 4);
		ImGui::SliderFloat3("translation", &m_Translation.x, -20.0f, 20.0f);
		ImGui::SliderFloat("rotation", &m_Rotation, 0.0f, 0.2f);
		ImGui::SliderFloat3("light_position", &m_LightPosition.x, -30.0f, 30.0f);
		ImGui::Checkbox("normal visualization", &m_NormalVisualizationFlag);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	}

	/* renderQuad() renders a 1x1 XY quad in NDC */
	unsigned int quadVAO = 0;
	unsigned int quadVBO;
	void TestMesh::renderQuad() {
		if (quadVAO == 0) {
			float quadVertices[] = {
				// positions        // texture Coords
				-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
				 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
				 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			};
			// setup plane VAO
			glGenVertexArrays(1, &quadVAO);
			glGenBuffers(1, &quadVBO);
			glBindVertexArray(quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		}
		glBindVertexArray(quadVAO);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glBindVertexArray(0);
	}


	// renders a 1x1 quad in NDC with manually calculated tangent vectors
// ------------------------------------------------------------------
	unsigned int quadNormalMapVAO = 0;
	unsigned int quadNormalMapVBO;

	void TestMesh::renderNormalMappedQuad() {
		if (quadVAO == 0) {
			// positions
			glm::vec3 pos1(-1.0f, 1.0f, 0.0f);
			glm::vec3 pos2(-1.0f, -1.0f, 0.0f);
			glm::vec3 pos3(1.0f, -1.0f, 0.0f);
			glm::vec3 pos4(1.0f, 1.0f, 0.0f);
			// texture coordinates
			glm::vec2 uv1(0.0f, 1.0f);
			glm::vec2 uv2(0.0f, 0.0f);
			glm::vec2 uv3(1.0f, 0.0f);
			glm::vec2 uv4(1.0f, 1.0f);
			// normal vector
			glm::vec3 nm(0.0f, 0.0f, 1.0f);

			// calculate tangent/bitangent vectors of both triangles
			glm::vec3 tangent1, bitangent1;
			glm::vec3 tangent2, bitangent2;
			// triangle 1
			// ----------
			glm::vec3 edge1 = pos2 - pos1;
			glm::vec3 edge2 = pos3 - pos1;
			glm::vec2 deltaUV1 = uv2 - uv1;
			glm::vec2 deltaUV2 = uv3 - uv1;

			GLfloat f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

			tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
			tangent1 = glm::normalize(tangent1);

			bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
			bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
			bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
			bitangent1 = glm::normalize(bitangent1);

			// triangle 2
			// ----------
			edge1 = pos3 - pos1;
			edge2 = pos4 - pos1;
			deltaUV1 = uv3 - uv1;
			deltaUV2 = uv4 - uv1;

			f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

			tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
			tangent2 = glm::normalize(tangent2);


			bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
			bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
			bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
			bitangent2 = glm::normalize(bitangent2);


			float quadVertices[] = {
				// positions            // normal         // texcoords  // tangent                          // bitangent
				pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
				pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
				pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,

				pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
				pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
				pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
			};
			// configure plane VAO
			glGenVertexArrays(1, &quadNormalMapVAO);
			glGenBuffers(1, &quadNormalMapVBO);
			glBindVertexArray(quadNormalMapVAO);
			glBindBuffer(GL_ARRAY_BUFFER, quadNormalMapVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
			glEnableVertexAttribArray(4);
			glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));


		
		}
		glBindVertexArray(quadNormalMapVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}


}



//if (useOrthographicFlag) {
//	m_Proj = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, near_plane, far_plane);
//} else {


			/*
			m_Shader->SetUniformMat4f("u_M1", glm::mat4(1.0f));
			m_Shader->SetUniformMat4f("u_M2", glm::mat4(1.0f));
			m_Shader->SetUniformMat4f("u_M3", glm::mat4(1.0f));
*/