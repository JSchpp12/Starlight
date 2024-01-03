#include "RenderResourceModifierGeometry.hpp"

void star::RenderResourceModifierGeometry::registerCallbacks()
{
	auto loadGeomCallback = std::function<std::pair<std::unique_ptr<StarBuffer>, std::unique_ptr<StarBuffer>>(StarDevice&, Handle&, Handle&)>(std::bind(&RenderResourceModifierGeometry::loadGeometryStagingBuffers, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	RenderResourceSystem::registerLoadGeomDataCallback(loadGeomCallback);
}
