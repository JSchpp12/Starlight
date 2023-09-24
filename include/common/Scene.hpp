#pragma once 

#include "Handle.hpp"

#include <vector>
namespace star {
class Scene {

public:
	Scene() = default; 
	virtual ~Scene() = default; 

protected:
	std::vector<Handle> objectList; 
	std::vector<Handle> lightList; 
};
}