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

	void createWindowSurface(vk::Instance instance, vk::UniqueSurfaceKHR& surface);

	bool shouldClose() { return glfwWindowShouldClose(this->window); }
	vk::Extent2D getExtent() { return { static_cast<uint32_t>(this->width), static_cast<uint32_t>(this->height) }; }
	bool wasWindowResized() { return this->frambufferResized; }
	void resetWindowResizedFlag() { this->frambufferResized = false; }
	GLFWwindow* getGLFWwindow() const { return this->window; }

protected:
	StarWindow(int width, int height, std::string title) : width(width), height(height), title(title) {};

	void onCreate();

	virtual void init() = 0;

	virtual void createWindow(); 

private: 
	bool frambufferResized = false;
	int width, height;
	std::string title;
	GLFWwindow* window = nullptr;


};
}