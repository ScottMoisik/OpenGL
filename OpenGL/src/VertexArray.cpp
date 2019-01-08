#include "Renderer.h"

#include "VertexBufferLayout.h"

VertexArray::VertexArray() {
	GLCall(glGenVertexArrays(1, &m_RendererID));
}

VertexArray::~VertexArray() {
	GLCall(glDeleteVertexArrays(1, &m_RendererID));
}

void VertexArray::AddBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout) {
	Bind();
	vb.Bind();
	const auto& elements = layout.GetElements();
	unsigned int offset = 0;
	for (unsigned int i = 0; i < elements.size(); i++) {
		const auto& element = elements[i];

		if (element.type != GL_MATRIX4_NV) {
			GLCall(glEnableVertexAttribArray(i));
			GLCall(glVertexAttribPointer(i, element.count, element.type, element.normalized, layout.GetStride(), (const void*)offset));
			offset += element.count * VertexBufferElement::GetSizeOfType(element.type);
		} else {
			/* Instance mode: Filling a matrix into the vertex buffer requires providing four vec 4s, one four each column */
			for (unsigned int j = 0; j < 4; j++) {
				glEnableVertexAttribArray(i + j);
				glVertexAttribPointer(i + j, element.count, GL_FLOAT, element.normalized, layout.GetStride(), (const void*)offset);
				glVertexAttribDivisor(i + j, 1); // Specify that this part of the buffer is accessed only for each complete instance (vs. every single vertex, = 0) 
				offset += (4 * sizeof(float));
			}
			
		}

		
	}
}

void VertexArray::Bind() const {
	GLCall(glBindVertexArray(m_RendererID));
}

void VertexArray::Unbind() const {
	GLCall(glBindVertexArray(0));
}
