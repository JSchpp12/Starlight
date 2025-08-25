#include "RenderResourceModifier.hpp"

#include "RenderResourceSystem.hpp"

#include <functional>

void star::RenderResourceModifier::registerRenderResourceCallbacks()
{
	auto initCallback = std::function<void(core::devices::DeviceContext&, const int&, const vk::Extent2D&)>(std::bind(&RenderResourceModifier::initResources, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	auto destroyCallback = std::function<void(core::devices::DeviceContext&)>(std::bind(&RenderResourceModifier::destroyResources, this, std::placeholders::_1)); 
	
	RenderResourceSystem::registerCallbacks(initCallback, destroyCallback); 
}
