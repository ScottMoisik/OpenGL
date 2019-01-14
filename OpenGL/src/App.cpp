#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <Windows.h>

#include "Renderer.h"

#include "VertexArray.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "IndexBuffer.h"
#include "Shader.h"
#include "Texture.h"

#include "tests/Test.h"
#include "tests/TestClearColor.h"
#include "tests/TestTexture2D.h"
#include "tests/TestMesh.h"
#include "tests/TestParticle.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw_gl3.h"
#include "App.h"

#include "Mesh.h"

/* Forward declarations of camera-related input callbacks */
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

Camera camera;
float lastX, lastY;
float deltaTime = 0.0f;	// time between current frame and last frame
float lastTime = 0.0f;
bool firstMouse = true;
bool mouseClickFlag = false;

#define BASE_DPI 72.0f

int main(void) {
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	int windowWidth = 3 * GetSystemMetrics(SM_CXSCREEN) / 4;
	int windowHeight = 3 * GetSystemMetrics(SM_CYSCREEN) / 4;
	
	


	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(windowWidth, windowHeight, "OpenGL Development", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	/* Get the monitor dpi */
	int count;
	GLFWmonitor* primary = glfwGetPrimaryMonitor();
	int widthMM, heightMM;
	const GLFWvidmode* mode = glfwGetVideoMode(primary);
	glfwGetMonitorPhysicalSize(primary, &widthMM, &heightMM);
	const double dpi = mode->width / (widthMM / 25.4);

	/* Set callbacks for input to control camera */
	camera = Camera(windowWidth, windowHeight, glm::vec3(0.0f, 10.0f, 15.0f));
	lastX = windowWidth / 2.0f;
	lastY = windowHeight / 2.0f;
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_move_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	if (glewInit() != GLEW_OK)
		std::cout << "Error!" << std::endl;

	std::cout << glGetString(GL_VERSION) << std::endl;

	{ //Prevent infinite loop in glGetError calls by ensuring destructor (which?) is called 

		GLCall(glEnable(GL_BLEND));
		GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
		
		Renderer renderer;

		// Setup Dear ImGui context
		ImGui::CreateContext();
		ImGui_ImplGlfwGL3_Init(window, false);

		ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(dpi/BASE_DPI);

		ImGuiIO& io = ImGui::GetIO();
		io.FontGlobalScale = dpi / BASE_DPI;

		Test::Test* currentTest = NULL;
		Test::TestMenu* testMenu = new Test::TestMenu(currentTest);
		currentTest = testMenu;

		testMenu->RegisterTest<Test::TestClearColor>("Clear Color");
		testMenu->RegisterTest<Test::TestTexture2D>("Texture 2D");
		testMenu->RegisterTest<Test::TestMesh>("Basic Mesh");
		testMenu->RegisterTest<Test::TestParticle>("Particles");


		while (!glfwWindowShouldClose(window)) {
			float currentTime = glfwGetTime();
			deltaTime = currentTime - lastTime;
			lastTime = currentTime;

			GLCall(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
			glCullFace(GL_BACK);


			renderer.Clear();

			ImGui_ImplGlfwGL3_NewFrame();
			processInput(window);


			if (currentTest) {
				currentTest->SetCamera(&camera);
				currentTest->SetScreenSize(windowWidth, windowHeight);
				currentTest->OnUpdate(currentTime);
				currentTest->OnRender();
				ImGui::Begin("Begin Test");
				ImGui::SetWindowPos(ImVec2(2.0f * windowWidth / 3.0f, 2.0f * windowHeight / 3.0f));
				ImGui::SetWindowSize(ImVec2(windowWidth / 3.0f, windowHeight / 3.0f));

				if (currentTest != testMenu && ImGui::Button("<-")) {
					delete currentTest;
					currentTest = testMenu;
					
				}
				currentTest->OnImGuiRender();
				ImGui::End();
			}

			ImGui::SetWindowSize(ImVec2((float)800, (float)800));

			ImGui::Render();
			ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());

			glfwSwapBuffers(window);
			glfwPollEvents();
		}

		delete currentTest;
		if (currentTest != testMenu) {
			delete testMenu;
		}
	}

	ImGui_ImplGlfwGL3_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}



/* process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly */
void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

/* glfw: whenever the window size changed (by OS or user resize) this callback function executes */
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	/* Make sure the viewport matches the new window dimensions; note that width and  height will be significantly larger than specified on retina displays. */
	glViewport(0, 0, width, height);
}


/* glfw: whenever the mouse scroll wheel scrolls, this callback is called */
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (action == GLFW_PRESS && button == 2) {
		mouseClickFlag = true;
	} else {
		mouseClickFlag = false;
		firstMouse = true;
	}
}

/* glfw: whenever the mouse moves, this callback is called */
void mouse_move_callback(GLFWwindow* window, double xpos, double ypos) {
	if (mouseClickFlag) {
		if (firstMouse) {
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

		lastX = xpos;
		lastY = ypos;

		camera.ProcessMouseMovement(xoffset, yoffset);
	}
}

/* glfw: whenever the mouse scroll wheel scrolls, this callback is called */
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	camera.ProcessMouseScroll(yoffset);
}