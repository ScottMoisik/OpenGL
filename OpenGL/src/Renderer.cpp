#include "Renderer.h"

#include <iostream>

void GLClearError() {
	//Continually call glGetError to clear all errors (each of which is retrieved arbitrarily)
	while (glGetError() != GL_NO_ERROR);
}

bool GLLogCall(const char* function, const char* file, int line) {
	while (GLenum error = glGetError()) {
		std::cout << "[OpenGL Error]: (" << error << "): " <<
			function << " " << file << ": " << line << std::endl;
		return false;
	};
	return true;
}

Renderer::Renderer() {
}

Renderer::~Renderer() {
}

void Renderer::Draw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader, const unsigned int numInstances) const {
	shader.Bind();
	va.Bind();
	ib.Bind();

	if (numInstances < 2) {
		GLCall(glDrawElements(GL_TRIANGLES, ib.GetCount(), GL_UNSIGNED_INT, nullptr));
	} 
	else {
		GLCall(glDrawElementsInstanced(GL_TRIANGLES, ib.GetCount(), GL_UNSIGNED_INT, 0, numInstances));
	}
}


void Renderer::Clear() {
	GLCall(glClear(GL_COLOR_BUFFER_BIT));
}

void Renderer::SetClearColor(float r, float g, float b) {
	GLCall(glClearColor(r, g, b, 1.0f));
}
