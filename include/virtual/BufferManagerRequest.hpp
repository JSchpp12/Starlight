#pragma once

#include "StarBuffer.hpp"
#include "BufferMemoryTransferRequest.hpp"

#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>

#include <optional>
#include <functional>

namespace star{
    class BufferManagerRequest {
    public:

    BufferManagerRequest() = default;

    BufferManagerRequest(const uint8_t& frameInFlightIndexToUpdateOn) 
    : frameInFlightIndexToUpdateOn(frameInFlightIndexToUpdateOn){}; 

    virtual std::unique_ptr<BufferMemoryTransferRequest> createTransferRequest() const = 0; 

    virtual bool isValid(const uint8_t& currentFrameInFlightIndex) const {
        if(frameInFlightIndexToUpdateOn.has_value() && currentFrameInFlightIndex == frameInFlightIndexToUpdateOn.value()) {
            return false;
        }

        return true;
    }

    const std::optional<uint8_t>& getFrameInFlightIndexToUpdateOn() const { return frameInFlightIndexToUpdateOn; }

    //If buffer needs to be recreated, should give a handle with the request
    //BufferManagerRequest(handle handle)

    protected:
    std::optional<uint8_t> frameInFlightIndexToUpdateOn = std::nullopt;
    };
}