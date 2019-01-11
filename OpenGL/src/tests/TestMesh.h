#pragma once

#include "Test.h"

#include "Mesh.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "Texture.h"

#include "Camera.h"

namespace Test {

	class TestMesh : public Test {
	public:
		TestMesh();
		~TestMesh();

		void OnUpdate(float deltaTime) override;
		void OnRender() override;
		void OnImGuiRender() override;
	private:
		glm::vec3 m_Translation, m_LightPosition;
		float m_Rotation;
		std::unique_ptr<Mesh> m_Mesh, m_MeshPlane, m_MeshLight;
		std::unique_ptr<VertexArray> m_VAO;
		std::unique_ptr<VertexBuffer> m_VertexBuffer;
		std::unique_ptr<IndexBuffer> m_IndexBuffer;
		std::unique_ptr<Shader> m_Shader, m_NormalVisualizingShader;
		std::unique_ptr<Texture> m_Texture, m_PlaneTexture, m_LightTexture;
		glm::mat4 m_Proj, m_View;
		float m_ViewPortWidth, m_ViewPortHeight, m_AspectRatio;

		bool m_NormalVisualizationFlag = false;


	};

}