#include "StarWindow.hpp"

namespace star {

void StarWindow::createWindow()
{
	glfwInit();
	//tell GLFW to create a window but to not include a openGL instance as this is a default behavior
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	//disable resizing functionality in glfw as this will not be handled in the first tutorial
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	this->window = glfwCreateWindow(width, height, this->title.c_str(), nullptr, nullptr);

	//need to give GLFW a pointer to current instance of this class
	glfwSetWindowUserPointer(this->window, this);

}

void StarWindow::createWindowSurface(vk::Instance instance, vk::UniqueSurfaceKHR& surface)
{
	VkSurfaceKHR surfaceTmp;
	if (glfwCreateWindowSurface(instance, this->window, nullptr, &surfaceTmp) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface");
	}
	surface = vk::UniqueSurfaceKHR(surfaceTmp, instance);
}

void StarWindow::onCreate()
{
	this->init(); 
	createWindow(); 
}

}