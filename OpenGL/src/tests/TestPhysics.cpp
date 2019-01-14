#include "TestPhysics.h"

#include "Renderer.h"
#include "Mesh.h"

#include "imgui/imgui.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>

#include "physics/Inertia.h"

namespace Test {
	std::unique_ptr<Mesh> m_Sphere;

	TestPhysics::TestPhysics() : 
		m_Proj(glm::perspective(glm::radians(45.0f), 3.0f / 4.0f, 0.1f, 100.0f)),
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

		//Load meshes
		m_Mesh = (std::unique_ptr<Mesh>)Mesh::Plane(1);
		m_Mesh->SetColor(0.2f, 0.2f, 0.6f, 1.0f);
		m_Texture = std::make_unique<Texture>("res/textures/marble.jpg");

		m_Sphere = (std::unique_ptr<Mesh>)Mesh::Sphere(Mesh::SphereDivisions::res32, 1);
		InertialProps ip = ComputeIntertiaProperties(*m_Sphere, 2.0f);

		// Load shaders for the scene
		m_BasicShader = std::make_unique<Shader>("res/shaders/Basic.shader");

		


	}


	TestPhysics::~TestPhysics() {
	}

	void TestPhysics::OnUpdate(float deltaTime) {
		m_Mesh->Update(deltaTime, 100.0f, glm::vec3(0.0f), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	}

	enum ProjectionType { ORTHO, PERSPECTIVE };
	void TestPhysics::OnRender() {
		GLCall(glClearColor(0.2f, 0.2f, 0.6f, 1.0f));
		GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

		ProjectionType projType = ProjectionType::ORTHO;

		{
			using namespace glm;

			glm::mat4 lightProjection, lightView, lightSpaceMatrix;
			float near_plane = -10.0f, far_plane = 10.0f;
			if (projType == ORTHO) {
				float range = 50.0;
				lightProjection = glm::ortho(-range, range, -range, range, near_plane, far_plane);
			} else {
				lightProjection = glm::perspective(glm::radians(45.0f), 1.0f, near_plane, far_plane);
			}
			
			/* Camera processing */
			m_Proj = glm::perspective(glm::radians(m_Camera->Zoom), m_Camera->m_AspectRatio, 1.0f, 1000.0f);
			m_View = m_Camera->GetViewMatrix();
			
			/* Set uniforms for the basic shader */
			mat4 model = translate(mat4(1.0f), m_LightPosition);
			mat4 MVP = m_Proj * m_View * model; //GLM is column-major memory layout so requires reverse multiplication for MVP
			MVP = m_Proj * m_View * model; //GLM is column-major memory layout so requires reverse multiplication for MVP
			
			// Set uniforms for the basic shader
			m_BasicShader->Bind();
			m_BasicShader->SetUniformMat4f("u_MVP", MVP);
			m_BasicShader->SetUniform1i("u_Texture", 0);
			m_Texture->Bind(0);
			m_Mesh->Draw(*m_BasicShader);
			
		}
	}

	void TestPhysics::OnImGuiRender() {
	}

	void TestPhysics::RenderScene() {
	}


}