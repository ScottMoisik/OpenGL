#include "TestMesh.h"


#include "Renderer.h"
#include "Mesh.h"

#include "imgui/imgui.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace Test {

	TestMesh::TestMesh() :
		m_Proj(glm::ortho(0.0f, 960.0f, 0.0f, 540.0f, -1.0f, 1.0f)),
		m_View(glm::translate(glm::mat4(1.0f), glm::vec3(-0.0f, 0.0f, 0.0f))),
		m_TranslationA(200.0f, 200.0f, 0.0f), m_TranslationB(400.0f, 200.0f, 0.0f) {

		GLCall(glEnable(GL_BLEND));
		GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

		m_Shader = std::make_unique<Shader>("res/shaders/BasicMesh.shader");
		m_Shader->Bind();
		m_Shader->SetUniform4f("u_Color", 0.8f, 0.3f, 0.8f, 1.0f);
		
		m_Mesh = std::make_unique<Mesh>("res/meshes/suzanne.obj");
	}


	TestMesh::~TestMesh() {}
	void TestMesh::OnUpdate(float deltaTime) {}
	void TestMesh::OnRender() {
		GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
		GLCall(glClear(GL_COLOR_BUFFER_BIT));

		Renderer renderer;

		//m_Texture->Bind();

		{
			glm::mat4 model = glm::translate(glm::mat4(1.0f), m_TranslationA);
			glm::mat4 MVP = m_Proj * m_View * model; //GLM is column-major memory layout so requires reverse multiplication for MVP
			m_Shader->Bind();
			m_Shader->SetUniformMat4f("u_MVP", MVP);

			m_Mesh->Draw(*m_Shader);
			//renderer.Draw(*m_VAO, *m_IndexBuffer, *m_Shader);
		}

	}
	void TestMesh::OnImGuiRender() {
		ImGui::SliderFloat3("translationA", &m_TranslationA.x, -200.0f, 200.0f);
		ImGui::SliderFloat3("translationB", &m_TranslationB.x, -200.0f, 200.0f);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	}
}