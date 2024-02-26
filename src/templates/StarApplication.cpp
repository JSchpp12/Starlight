#include "StarApplication.hpp"

std::unique_ptr<star::SwapChainRenderer> star::StarApplication::getMainRenderer(star::StarDevice& device, star::StarWindow& window) {
	std::vector<std::unique_ptr<Light>>& lightList = scene.getLights(); 
	std::vector<std::reference_wrapper<StarObject>> prepObjects; 
	for (auto& obj : scene.getObjects()) {
		prepObjects.push_back(*obj.second); 
	}
	return std::unique_ptr<star::SwapChainRenderer>(new star::SwapChainRenderer(window, lightList, prepObjects, camera, device));
}
