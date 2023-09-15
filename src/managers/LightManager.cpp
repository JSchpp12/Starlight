#include "LightManager.hpp"

namespace star {

Handle LightManager::createAppropriateHandle() {
	Handle newHandle;
	newHandle.type = Handle_Type::light;
	return newHandle;
}

}