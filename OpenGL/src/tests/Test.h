#pragma once


#include <vector>
#include <functional>

class Camera;

namespace Test {

	class Test {
	public:
		Test() {}
		virtual ~Test() {}

		virtual void OnUpdate(float deltaTime) {}
		virtual void OnRender() {}
		virtual void OnImGuiRender() {}
		void SetCamera(Camera* camera) { m_Camera = camera; }
		void SetScreenSize(int width, int height) { m_Width = width; m_Height = height; }
		
	protected:
		Camera* m_Camera;
		int m_Width, m_Height;
	};


	class TestMenu : public Test {
	public:
		TestMenu(Test*& currentTestPointer);
		virtual ~TestMenu() {}

		template<typename T>
		void RegisterTest(const std::string& name) {
			std::cout << "Registering test " << name << std::endl;
			m_Tests.push_back(std::make_pair(name, []() { return new T(); }));
		}

		void OnImGuiRender() override;
	private:
		Test*& m_CurrentTest;
		std::vector<std::pair<std::string, std::function<Test*()>>> m_Tests;
	};

}