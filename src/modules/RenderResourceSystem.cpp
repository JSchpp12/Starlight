#include "RenderResourceSystem.hpp"

std::stack<std::function<void(const int)>> star::RenderResourceSystem::initCallbacks = std::stack<std::function<void(int)>>();

void star::RenderResourceSystem::registerPreRendererInitResource(std::function<void(const int)> initCallback)
{
	initCallbacks.push(initCallback); 
}

void star::RenderResourceSystem::runInits(const int numFramesInFlight)
{
	while (!initCallbacks.empty()) {
		std::function<void(const int)>& function = initCallbacks.top(); 
		function(numFramesInFlight); 
		initCallbacks.pop(); 
	}
}
