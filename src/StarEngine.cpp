#include "StarEngine.hpp"

namespace star {

StarEngine::StarEngine() : currentScene(std::unique_ptr<StarScene>(new StarScene())) {
	ConfigFile::load("./StarEngine.cfg"); 
}

void StarEngine::Run()
{
	mainRenderer->prepare();

	while (!window->shouldClose()) {
		mainRenderer->pollEvents();
		InteractionSystem::callWorldUpdates();
		mainRenderer->submit();
	}
}

void StarEngine::init(StarApplication& app, RenderOptions& renderOptions) {
	//parse light information
	this->window = BasicWindow::New(800, 600, app.getApplicationName());

	this->renderingDevice = StarDevice::New(*window);

	this->mainRenderer = app.getMainRenderer(*this->renderingDevice, *this->window, renderOptions); 
}
}