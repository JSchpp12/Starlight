#include "BasicWindow.hpp"

namespace star {

std::unique_ptr<BasicWindow> BasicWindow::New(int width, int height, std::string title)
{
	return std::unique_ptr<BasicWindow>(new BasicWindow(width, height, title));
}

}