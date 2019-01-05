#pragma once

#include "Test.h"

#include "Mesh.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "Texture.h"

namespace Test {

	class TestMesh : public Test {
	public:
		TestMesh();
		~TestMesh();

		void OnUpdate(float deltaTime) override;
		void OnRender() override;
		void OnImGuiRender() override;
	private:
		glm::vec3 m_TranslationA, m_TranslationB;
		std::unique_ptr<Mesh> m_Mesh;
		std::unique_ptr<VertexArray> m_VAO;
		std::unique_ptr<VertexBuffer> m_VertexBuffer;
		std::unique_ptr<IndexBuffer> m_IndexBuffer;
		std::unique_ptr<Shader> m_Shader;
		std::unique_ptr<Texture> m_Texture;
		glm::mat4 m_Proj, m_View;
	};

}