#pragma once

#include <vector>
#include "Renderer.h"

struct VertexBufferElement {
	unsigned int type;
	unsigned int count;
	unsigned char normalized;

	static unsigned int GetSizeOfType(unsigned int type) {
		switch (type) {
			case GL_MATRIX4_NV:			return 4*4*4; //Size of float * 4 elements per vector * 4 vectors for a 4x4 matrix
			case GL_FLOAT:				return 4;
			case GL_UNSIGNED_INT:		return 4;
			case GL_UNSIGNED_BYTE:		return 1;
		}
		ASSERT(false);
		return 0;
	}
};

class VertexBufferLayout {
private:
	std::vector<VertexBufferElement> m_Elements;
	unsigned int m_Stride;
public:
	VertexBufferLayout() : m_Stride(0) {}
	~VertexBufferLayout() {}

	template<typename T>
	void Push(unsigned int count) {
		static_assert(false);
	}

	template<>
	void Push<float>(unsigned int count) {
		m_Elements.push_back({ GL_FLOAT, count, GL_FALSE });
		m_Stride += count * VertexBufferElement::GetSizeOfType(GL_FLOAT);
	}

	template<>
	void Push<unsigned int>(unsigned int count) {
		m_Elements.push_back({ GL_UNSIGNED_INT, count, GL_FALSE });
		m_Stride += count * VertexBufferElement::GetSizeOfType(GL_UNSIGNED_INT);
	}

	template<>
	void Push<unsigned char>(unsigned int count) {
		m_Elements.push_back({ GL_UNSIGNED_BYTE, count, GL_TRUE });
		m_Stride += count * VertexBufferElement::GetSizeOfType(GL_UNSIGNED_BYTE);
	}

	template<>
	void Push<glm::mat4>(unsigned int count) {
		m_Elements.push_back({ GL_MATRIX4_NV, 4*count, GL_FALSE });
		m_Stride += 64;
	}

	inline const std::vector<VertexBufferElement> GetElements() const { return m_Elements; }
	inline unsigned int GetStride() const { return m_Stride; }

};

