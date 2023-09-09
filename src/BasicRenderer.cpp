#include "BasicRenderer.hpp"

namespace star {
BasicRenderer::BasicRenderer(StarWindow& window) : window(window)
{
	device = StarDevice::New(window); 
}
}