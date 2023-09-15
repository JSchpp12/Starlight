#pragma once 
#include "Texture.hpp"
#include "StarMemoryManager.hpp"
#include "Handle.hpp"

#include <memory>

namespace star {

class MapManager : public StarMemoryManager<Texture> {
public:
	MapManager(std::unique_ptr<Texture> defaultMap);

protected:
	virtual Handle createAppropriateHandle() override;

private:


};

}