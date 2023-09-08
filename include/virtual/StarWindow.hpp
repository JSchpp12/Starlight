#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <memory>
#include <string>

#include <vulkan/vulkan.hpp>

namespace star {
class StarWindow {
public:
	virtual ~StarWindow() {
		glfwDestroyWindow(this->window); 
		glfwTerminate(); 
	};

	StarWindow(const StarWindow&) = delete; 
	StarWindow& operator=(const StarWindow&) = delete; 

	void createWindowSurface(vk::Instance instance, vk::UniqueSurfaceKHR& surface);

protected:
	StarWindow(int width, int height, std::string title) : width(width), height(height), title(title) {};

	void create(); 

	virtual void init() = 0;

	virtual void createWindow(); 

private: 
	int width, height;
	std::string title;
	GLFWwindow* window = nullptr;

};
}