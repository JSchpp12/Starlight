#include "StarEngine.hpp"

namespace star {

MapManager StarEngine::mapManager = MapManager(std::make_unique<Texture>(1, 1));

StarEngine::StarEngine() : currentScene(std::unique_ptr<StarScene>(new StarScene())) {
	ConfigFile::load("./StarEngine.cfg"); 
}

void StarEngine::Run()
{
	renderer->prepare();

	while (!window->shouldClose()) {
		renderer->pollEvents();
		InteractionSystem::callWorldUpdates();
		renderer->draw();
	}
}

void StarEngine::init(StarApplication& app, RenderOptions& renderOptions) {
	//parse light information
	this->window = BasicWindow::New(800, 600, app.getApplicationName());

	this->renderingDevice = StarDevice::New(*window);

	this->renderer = app.getRenderer(*renderingDevice, *window, renderOptions); 
}
}