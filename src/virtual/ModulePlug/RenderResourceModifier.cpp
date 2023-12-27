#include "RenderResourceModifier.hpp"

void star::RenderResourceModifier::registerCallbacks()
{
	auto initCallback = std::function<void(int)>(std::bind(&RenderResourceModifier::initResources, this, std::placeholders::_1));
	RenderResourceSystem::registerPreRendererInitResource(initCallback); 
}
