#pragma once

#include "device/StarDevice.hpp"

#include <starlight/common/Handle.hpp>
#include <starlight/common/HandleTypeRegistry.hpp>

namespace star
{
struct BufferHandle : public Handle
{
    BufferHandle(uint32_t id, size_t targetBufferOffset)
        : Handle(common::HandleTypeRegistry::instance().getType(common::special_types::BufferTypeName).value(), id),
          targetBufferOffset(std::move(targetBufferOffset))
    {
    }
    BufferHandle(uint32_t id)
        : Handle(common::HandleTypeRegistry::instance().getType(common::special_types::BufferTypeName).value(), id){};

    size_t targetBufferOffset = 0;
};
} // namespace star