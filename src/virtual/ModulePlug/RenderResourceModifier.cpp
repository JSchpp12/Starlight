#include "RenderResourceModifier.hpp"

void star::RenderResourceModifier::registerCallbacks()
{
	auto initCallback = std::function<void(StarDevice&, const int)>(std::bind(&RenderResourceModifier::initResources, this, std::placeholders::_1, std::placeholders::_2));
	auto destroyCallback = std::function<void(StarDevice&)>(std::bind(&RenderResourceModifier::destroyResources, this, std::placeholders::_1)); 

	RenderResourceSystem::registerCallbacks(initCallback, destroyCallback); 
}
