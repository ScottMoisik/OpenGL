#include "TestMesh.h"

#include "Renderer.h"
#include "Mesh.h"

#include "imgui/imgui.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace Test {

	TestMesh::TestMesh() :
		m_Proj(glm::perspective(glm::radians(45.0f), 3.0f/4.0f, 0.1f, 100.0f)),
		m_View(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -10.0f))),
		m_Translation(0.0f, 0.0f, 0.0f), m_LightPosition(0.0f, 10.0f, 10.0f) {
		

		GLint m_viewport[4];
		GLCall(glGetIntegerv(GL_VIEWPORT, m_viewport));
		m_ViewPortWidth = (float)m_viewport[2];
		m_ViewPortHeight = (float)m_viewport[3];
		m_AspectRatio = m_ViewPortWidth / m_ViewPortHeight;

		GLCall(glEnable(GL_BLEND));
		GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
		
		/* Enable depth testing */
		GLCall(glEnable(GL_DEPTH_TEST));

		/* Load primary shader for the scene */
		m_Shader = std::make_unique<Shader>("res/shaders/BasicMesh.shader");
		m_Shader->Bind();
		m_Shader->SetUniform4f("u_ObjectColor", 0.8f, 0.3f, 0.8f, 1.0f);

		/* Setup for lighting */
		m_Shader->SetUniform3f("u_LightColor", 0.8f, 0.8f, 0.8f);

		/* Load normal visualizing shader */
		m_NormalVisualizingShader = std::make_unique<Shader>("res/shaders/NormalVisualization.shader");
		m_NormalVisualizingShader->Bind();



		m_Mesh = std::make_unique<Mesh>("res/meshes/suzanne.obj");
	}


	TestMesh::~TestMesh() {}
	void TestMesh::OnUpdate(float deltaTime) {}
	void TestMesh::OnRender() {
		
		GLCall(glClearColor(0.2f, 0.2f, 0.6f, 1.0f));
		GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
		
		Renderer renderer;

		//m_Texture->Bind();
		m_Proj = glm::perspective(glm::radians(45.0f), m_AspectRatio, 0.1f, 100.0f);
		{
			using namespace glm;
			mat4 model = translate(mat4(1.0f), m_Translation);
			model = rotate(model, radians(m_Rotation), vec3(0.0f, 1.0f, 0.0f));
			mat4 MVP = m_Proj * m_View * model; //GLM is column-major memory layout so requires reverse multiplication for MVP
			m_Shader->Bind();
			m_Shader->SetUniformMat4f("u_Model", model);
			m_Shader->SetUniformMat4f("u_MVP", MVP);
			m_Shader->SetUniform3f("u_LightPosition", m_LightPosition.x, m_LightPosition.y, m_LightPosition.z);

			/* Set uniforms for the normal visualizing shader */
			m_NormalVisualizingShader->Bind();
			m_NormalVisualizingShader->SetUniformMat4f("u_Proj", m_Proj);
			m_NormalVisualizingShader->SetUniformMat4f("u_View", m_View);
			m_NormalVisualizingShader->SetUniformMat4f("u_Model", model);
			m_NormalVisualizingShader->SetUniformMat4f("u_MVP", MVP);

			/* Do the draw calls for each shader */
			m_Mesh->Draw(*m_Shader);
			//m_Mesh->Draw(*m_NormalVisualizingShader);
			//renderer.Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
		}

	}
	void TestMesh::OnImGuiRender() {
		ImGui::SliderFloat3("translation", &m_Translation.x, -20.0f, 20.0f);
		ImGui::SliderFloat("rotation", &m_Rotation, -180.0f, 180.0f);
		ImGui::SliderFloat3("light_position", &m_LightPosition.x, -20.0f, 20.0f);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	}
}