#pragma once 


#include <stack>
#include <functional>

namespace star {
	class RenderResourceSystem {
	public:
		static void registerPreRendererInitResource(std::function<void(const int)> initCallback);

		friend class StarEngine;
	protected:
		static std::stack<std::function<void(const int)>> initCallbacks; 

	private: 
		static void runInits(const int numFramesInFlight); 
	};
}