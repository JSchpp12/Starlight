#include "BasicWindow.hpp"

namespace star {

std::unique_ptr<BasicWindow> BasicWindow::New(int width, int height, std::string title)
{
	std::unique_ptr<BasicWindow> newWindow = std::unique_ptr<BasicWindow>(new BasicWindow(width, height, title));
	newWindow->onCreate(); 
	return std::move(newWindow); 
}

}