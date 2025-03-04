#pragma once

#include "StarBuffer.hpp"
#include "ManagerController.hpp"
#include "BufferMemoryTransferRequest.hpp"

#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>

#include <optional>
#include <functional>

namespace star{
    class BufferManagerRequest : public ManagerController {
    public:

    BufferManagerRequest() = default;

    BufferManagerRequest(const uint8_t& frameInFlightIndexToUpdateOn) 
    : ManagerController(frameInFlightIndexToUpdateOn){}; 

    virtual std::unique_ptr<BufferMemoryTransferRequest> createTransferRequest() const = 0; 

    protected:

    };
}