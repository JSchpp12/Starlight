#include "StarApplication.hpp"

std::unique_ptr<star::SwapChainRenderer> star::StarApplication::getMainRenderer(star::StarDevice& device, star::StarWindow& window) {
	std::vector<std::unique_ptr<Light>>& lightList = scene.getLights(); 
	std::vector<std::reference_wrapper<StarObject>> objects = scene.getObjects();
	return std::unique_ptr<star::SwapChainRenderer>(new star::SwapChainRenderer(window, lightList, objects, camera, device));
}
