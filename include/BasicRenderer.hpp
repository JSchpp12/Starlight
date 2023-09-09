#pragma once

#include "StarRenderer.hpp"
#include "StarWindow.hpp"

#include "StarDevice.hpp"

#include <memory>

namespace star {
class BasicRenderer : public StarRenderer {
public:
	BasicRenderer(StarWindow& window); 

	virtual void draw() {}; 

protected: 
	StarWindow& window; 
	std::unique_ptr<StarDevice> device; 

private:

};
}