#pragma once 
#include "Enums.hpp"

#include <optional>
#include <stdint.h>

namespace star {
    struct Handle {
        static Handle getDefault() {
            Handle newHandle;
            newHandle.type = Handle_Type::defaultHandle;
            return newHandle;
        }
        Handle() = default;
        Handle(const Handle& handle) = default;
        Handle(const size_t id, const Handle_Type type) : id(id), type(type) {};

        size_t id = 0;
        size_t containerIndex = 0;
        Handle_Type type = Handle_Type::null;
        bool isOnDisk = false;
        std::optional<Shader_Stage> shaderStage;
    };
}