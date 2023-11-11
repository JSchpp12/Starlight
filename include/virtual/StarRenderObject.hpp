#pragma once 

#include "StarObject.hpp"
#include "StarRenderer.hpp"

namespace star {
	//Object which will provide its own renderer in order to be drawn. 
	class StarRenderObject : private StarObject {
	public:
		StarRenderer& getRenderer() {
			return *this->renderer; 
		};

	protected:
		std::unique_ptr<StarRenderer> renderer; 
	};
}