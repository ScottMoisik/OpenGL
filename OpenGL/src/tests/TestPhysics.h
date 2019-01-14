#pragma once
#include "Test.h"

#include "Mesh.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "Texture.h"

#include "Camera.h"

namespace Test {

	class TestPhysics : public Test {
	public:
		TestPhysics();
		~TestPhysics();

		void OnUpdate(float deltaTime) override;
		void OnRender() override;
		void OnImGuiRender() override;
		void RenderScene();

	private:
		glm::vec3 m_Translation, m_LightPosition;
		float m_Rotation;
		std::unique_ptr<Mesh> m_Mesh;
		std::unique_ptr<VertexArray> m_VAO;
		std::unique_ptr<VertexBuffer> m_VertexBuffer;
		std::unique_ptr<IndexBuffer> m_IndexBuffer;
		std::unique_ptr<Shader> m_BasicShader;
		std::unique_ptr<Texture> m_Texture;
		glm::mat4 m_Proj, m_View;
		float m_ViewPortWidth, m_ViewPortHeight, m_AspectRatio;

	};

}