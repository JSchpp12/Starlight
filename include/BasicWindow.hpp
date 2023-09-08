#pragma once 

#include "StarWindow.hpp"

#include <memory>
#include <string>
#include <vulkan/vulkan.hpp>

namespace star {
class BasicWindow : public StarWindow {
public: 
	static std::unique_ptr<BasicWindow> New(int width, int height, std::string title);

	//no copy 
	BasicWindow(const BasicWindow&) = delete;
	BasicWindow& operator=(const BasicWindow&) = delete;

protected:
	BasicWindow(int width, int height, std::string title) : StarWindow(width, height, title) {}; 

	virtual void init() {}; 

private: 

};
}