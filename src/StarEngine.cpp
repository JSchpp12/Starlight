#include "StarEngine.hpp"

namespace star {

StarEngine::StarEngine(std::vector<Handle> lightHandles, LightManager* lightManager) : lightHandles(lightHandles){
	//parse light information
	this->window = BasicWindow::New(800, 600, "Test");
	auto renderBuilder = BasicRenderer::Builder(*this->window);

	std::vector<Light*> lightList = std::vector<Light*>(lightHandles.size());
	for (int i = 0; i < lightHandles.size(); i++) {
		renderBuilder.addLight(&lightManager->resource(lightHandles[i])); 
	}
	
	this->renderer = renderBuilder.build(); 
}
}