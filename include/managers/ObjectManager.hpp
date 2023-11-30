#pragma once

#include "Handle.hpp"
#include "StarObject.hpp"

#include <tiny_obj_loader.h>
#include <glm/glm.hpp>

#include <string>
#include <vector> 
#include <memory> 

namespace star {
    //todo: inherit from manager base
    class ObjectManager {
    public:
        StarObject& resource(const Handle& resourceHandle);

        Handle add(std::unique_ptr<StarObject> newObject); 
    
    private: 
        std::vector<std::unique_ptr<StarObject>> objects; 

    };
}
