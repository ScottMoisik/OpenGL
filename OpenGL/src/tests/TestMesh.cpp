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
		m_Translation(0.0f, 0.0f, 0.0f), m_LightPosition(0.0f, 15.0f, 0.0f) {
				
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
		
		m_MeshLight = (std::unique_ptr<Mesh>)Mesh::Cube(1);
		m_MeshLight->SetColor(0.9f, 0.9f, 1.0f, 1.0f);
		m_LightTexture = std::make_unique<Texture>("res/textures/azula.jpg");
		
		m_MeshBox = (std::unique_ptr<Mesh>)Mesh::Cube(1);
		m_MeshBox->SetColor(0.9f, 0.9f, 1.0f, 1.0f);

		m_MeshPlane = (std::unique_ptr<Mesh>)Mesh::Plane(1);
		m_MeshPlane->SetColor(0.2f, 0.2f, 0.6f, 1.0f);
		m_PlaneTexture = std::make_unique<Texture>("res/textures/marble.jpg");

 		m_Mesh = std::make_unique<Mesh>("res/meshes/earth.obj", numInstances);
		m_Texture = std::make_unique<Texture>("res/textures/earth.jpg");

		/* Load primary shader for the scene */
		m_Shader = std::make_unique<Shader>("res/shaders/BasicMeshInstanced.shader");
		m_Shader->Bind();
		m_Shader->SetUniform3f("u_LightColor", 0.6f, 0.6f, 0.6f);
		m_Shader->SetUniform4f("u_ObjectColor", 0.8f, 0.3f, 0.8f, 1.0f);
		m_Shader->SetUniform1i("u_Texture", 0);
		m_Shader->SetUniform1i("u_ShadowMap", 1);
		if (m_Texture != nullptr) {
			m_Shader->SetUniform1b("u_UseTexturing", true);
		} else {
			m_Shader->SetUniform1b("u_UseTexturing", false);
		}
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


		/* configure depth map FBO */
		glGenFramebuffers(1, &depthMapFBO);

		// create depth texture
		glGenTextures(1, &depthMap);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		
		/* attach depth texture as FBO's depth buffer */
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}


	TestMesh::~TestMesh() {}
	void TestMesh::OnUpdate(float deltaTime) {
		/* Define instance matrices */
		m_Mesh->m_InstanceModelMatrices.clear();
		m_Mesh->m_InstanceMVPMatrices.clear();
		float angularVel = 3.14f / 10.0f;
		for (unsigned int i = 0; i < m_Mesh->m_NumInstances; i++) {
			m_Mesh->m_InstanceModelMatrices.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(0.5f*(float)i, 3.0f*(float)i, 0.0f)));
			m_Mesh->m_InstanceMVPMatrices.push_back(glm::rotate(glm::mat4(1.0f), angularVel*((float)deltaTime), glm::vec3(0.0f, 1.0f, 0.0f)));
		}
		m_Mesh->Update();

		float scale = 100.0f;
		m_MeshPlane->m_InstanceModelMatrices.clear();
		m_MeshPlane->m_InstanceMVPMatrices.clear();
		m_MeshPlane->m_InstanceModelMatrices.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)));
		m_MeshPlane->m_InstanceMVPMatrices.push_back(glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, scale)));
		m_MeshPlane->Update();

		float boxScale = 2.0f;
		m_MeshBox->m_InstanceModelMatrices.clear();
		m_MeshBox->m_InstanceMVPMatrices.clear();
		m_MeshBox->m_InstanceModelMatrices.push_back(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
		m_MeshBox->m_InstanceMVPMatrices.push_back(glm::scale(glm::mat4(1.0f), glm::vec3(boxScale, boxScale, boxScale)));
		m_MeshBox->Update();


		float lightScale = 1.0f;
		m_MeshLight->m_InstanceModelMatrices.clear();
		m_MeshLight->m_InstanceMVPMatrices.clear();
		m_MeshLight->m_InstanceModelMatrices.push_back(glm::translate(glm::mat4(1.0f), m_LightPosition));
		m_MeshLight->m_InstanceMVPMatrices.push_back(glm::scale(glm::mat4(1.0f), glm::vec3(lightScale, lightScale, lightScale)));
		m_MeshLight->Update();
	}
	void TestMesh::OnRender() {
		GLCall(glClearColor(0.2f, 0.2f, 0.6f, 1.0f));
		GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
		
		bool useOrthographicFlag = true;

		{
			using namespace glm;
			/* 1: Shadown rendering: Set up shadow rendering: render depth of scene from light's perspective */
			
			glm::mat4 lightProjection, lightView, lightSpaceMatrix;
			float near_plane = -10.0f, far_plane = 10.0f;
			if (useOrthographicFlag) {
				lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
			} else {
				lightProjection = glm::perspective(glm::radians(m_Camera->Zoom), 1.0f, near_plane, far_plane);
			}

			
			lightView = glm::lookAt(m_LightPosition, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0, 1.0, 0.0));
			//lightView = m_Camera->GetViewMatrix();
			lightSpaceMatrix = lightProjection * lightView;

			glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
			glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
			glClear(GL_DEPTH_BUFFER_BIT);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, depthMap);

			mat4 model = translate(mat4(1.0f), m_Translation);
			model = rotate(model, radians(m_Rotation), vec3(0.0f, 1.0f, 0.0f));

			m_DepthShader->Bind();
			m_DepthShader->SetUniformMat4f("u_LightSpaceMatrix", lightSpaceMatrix);
			
			m_MeshPlane->Draw(*m_DepthShader);
			m_MeshLight->Draw(*m_DepthShader);
			m_MeshBox->Draw(*m_DepthShader);
			m_Mesh->Draw(*m_DepthShader);

			glBindFramebuffer(GL_FRAMEBUFFER, 0); //Return to default frame buffer
			

			/* 2: Ordinary rendering with shadows determined from depth map */
			glViewport(0, 0, m_Width, m_Height); //SCR_WIDTH, SCR_HEIGHT);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			/* Camera processing */
			m_Shader->Bind();
			
			//if (useOrthographicFlag) {
			//	m_Proj = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, near_plane, far_plane);
			//} else {
				m_Proj = glm::perspective(glm::radians(m_Camera->Zoom), m_Camera->m_AspectRatio, 1.0f, 1000.0f);
			//}
			m_View = m_Camera->GetViewMatrix();
			
			/* Set uniforms for the basic shader */
			mat4 MVP = m_Proj * m_View * model; //GLM is column-major memory layout so requires reverse multiplication for MVP
			
			/*
			m_Shader->SetUniformMat4f("u_M1", glm::mat4(1.0f));
			m_Shader->SetUniformMat4f("u_M2", glm::mat4(1.0f));
			m_Shader->SetUniformMat4f("u_M3", glm::mat4(1.0f));
*/
			glm::mat4 biasMatrix(
				0.5, 0.0, 0.0, 0.0,
				0.0, 0.5, 0.0, 0.0,
				0.0, 0.0, 0.5, 0.0,
				0.5, 0.5, 0.5, 1.0
			);

			glm::mat4 depthBiasMVP = biasMatrix * lightSpaceMatrix;


			m_Shader->SetUniformMat4f("u_MVP", MVP);
			m_Shader->SetUniformMat4f("u_Proj", m_Proj);
			m_Shader->SetUniformMat4f("u_View", m_View);
			m_Shader->SetUniformMat4f("u_LightSpaceMatrix", depthBiasMVP);
			

			


			m_Shader->SetUniform3f("u_ViewPos", m_Camera->Position.x, m_Camera->Position.y, m_Camera->Position.z);
			m_Shader->SetUniform3f("u_LightPosition", m_LightPosition.x, m_LightPosition.y, m_LightPosition.z);
	

			/* Setup textures for the light */
			if (m_LightTexture != nullptr) {
				//m_LightTexture->Bind();
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, depthMap);
				m_Shader->SetUniform1b("u_UseTexturing", true);
			}
			else {
				m_Shader->SetUniform1b("u_UseTexturing", false);
			}

			m_MeshLight->Draw(*m_Shader);
			m_MeshBox->Draw(*m_Shader);
			m_LightTexture->Unbind();

			/* Setup textures for the mesh */
			if (m_Texture != nullptr) {
				//m_Texture->Bind();
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, depthMap);
				m_Shader->SetUniform1b("u_UseTexturing", true);
			} else {
				m_Shader->SetUniform1b("u_UseTexturing", false);
			}

			/* Do draw call for mesh shader*/
			m_Mesh->Draw(*m_Shader);

			/* Setup textures for the plane */
			if (m_PlaneTexture != nullptr) {
				//m_PlaneTexture->Bind();
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, depthMap);
				
				m_Shader->SetUniform1b("u_UseTexturing", true);
			}
			else {
				m_Shader->SetUniform1b("u_UseTexturing", false);
			}

			m_MeshPlane->Draw(*m_Shader);
			m_PlaneTexture->Unbind();


			if (m_NormalVisualizationFlag) { 
				/* Set uniforms for the normal visualizing shader */
				m_NormalVisualizingShader->Bind();
				m_NormalVisualizingShader->SetUniformMat4f("u_Proj", m_Proj);
				m_NormalVisualizingShader->SetUniformMat4f("u_View", m_View);
				m_NormalVisualizingShader->SetUniformMat4f("u_Model", model);
				m_NormalVisualizingShader->SetUniformMat4f("u_MVP", MVP);

				/* Do draw call for normal visualizing shader*/
				m_Mesh->Draw(*m_NormalVisualizingShader); 
				m_MeshLight->Draw(*m_NormalVisualizingShader);
				m_NormalVisualizingShader->Unbind();
			}

			/* render depth map to quad for visual debugging */
			glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
			m_DebugDepthQuadShader->Bind();
			m_DebugDepthQuadShader->SetUniform1b("u_OrthographicFlag", true);
			m_DebugDepthQuadShader->SetUniform1f("u_NearPlane", near_plane);
			m_DebugDepthQuadShader->SetUniform1f("u_FarPlane", far_plane);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, depthMap);
			renderQuad();

			
		}

	}


	void TestMesh::RenderScene() {

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

	void TestMesh::OnImGuiRender() {
		ImGui::SliderFloat3("translation", &m_Translation.x, -20.0f, 20.0f);
		ImGui::SliderFloat("rotation", &m_Rotation, -180.0f, 180.0f);
		ImGui::SliderFloat3("light_position", &m_LightPosition.x, -30.0f, 30.0f);
		ImGui::Checkbox("normal visualization", &m_NormalVisualizationFlag);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	}
}