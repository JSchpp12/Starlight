#include "StarEngine.hpp"

namespace star {
StarEngine::~StarEngine()
{
}
StarEngine::StarEngine(){
	this->window = BasicWindow::New(800, 600, "Test");
}
}