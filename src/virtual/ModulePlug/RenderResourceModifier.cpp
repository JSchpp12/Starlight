#include "RenderResourceModifier.hpp"

void star::RenderResourceModifier::registerRenderResourceCallbacks()
{
	auto initCallback = std::function<void(StarDevice&, const int&, const vk::Extent2D&)>(std::bind(&RenderResourceModifier::initResources, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	auto destroyCallback = std::function<void(StarDevice&)>(std::bind(&RenderResourceModifier::destroyResources, this, std::placeholders::_1)); 
	
	RenderResourceSystem::registerCallbacks(initCallback, destroyCallback); 
}
