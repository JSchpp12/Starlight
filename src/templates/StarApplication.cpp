#include "StarApplication.hpp"

std::unique_ptr<star::StarRenderer> star::StarApplication::getRenderer(star::StarDevice& device, star::StarWindow& window, RenderOptions& options ) {
	std::vector<std::unique_ptr<Light>>& lightList = scene.getLights(); 
	std::vector<std::reference_wrapper<StarObject>> prepObjects; 
	for (auto& obj : scene.getObjects()) {
		prepObjects.push_back(*obj.second); 
	}
	return std::unique_ptr<star::BasicRenderer>(new star::BasicRenderer(window, lightList, prepObjects, camera, options, device));
}
