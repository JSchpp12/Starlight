//#include "MapManager.hpp"
//
//star::MapManager::MapManager(std::unique_ptr<Texture> defaultMap) : StarMemoryManager<Texture>() {
//	this->StarMemoryManager<Texture>::init(std::move(defaultMap));
//}
//
//star::Handle star::MapManager::createAppropriateHandle() {
//	Handle newHandle;
//	newHandle.type = Handle_Type::map;
//	return newHandle;
//}