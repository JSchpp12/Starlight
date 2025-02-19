#pragma once

#include "StarBuffer.hpp"
#include "MemoryTransferRequest.hpp"

#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>
#include <boost/atomic.hpp>

#include <optional>
#include <functional>

namespace star{
    class BufferManagerRequest {
public:
        struct BufferCreationArgs {
            vk::DeviceSize instanceSize;
            uint32_t instanceCount;
            VmaAllocationCreateFlags creationFlags;
            VmaMemoryUsage memoryUsageFlags;
            vk::BufferUsageFlags useFlags;
            vk::SharingMode sharingMode;

            BufferCreationArgs(const vk::DeviceSize& instanceSize,
                const uint32_t& instanceCount, const VmaAllocationCreateFlags& creationFlags, const VmaMemoryUsage& memoryUsageFlags,
                const vk::BufferUsageFlags& useFlags, const vk::SharingMode& sharingMode) 
                : instanceSize(instanceSize), instanceCount(instanceCount), creationFlags(creationFlags), memoryUsageFlags(memoryUsageFlags),
                useFlags(useFlags), sharingMode(sharingMode){};
        };

        BufferManagerRequest(const BufferCreationArgs& creationArgs) 
        : creationArgs(creationArgs){}

        BufferManagerRequest(const BufferCreationArgs& creationArgs, const uint8_t& frameInFlightIndexToUpdateOn) 
        :  creationArgs(creationArgs), frameInFlightIndexToUpdateOn(frameInFlightIndexToUpdateOn){}; 

        ///Function to handle updating buffer data
        virtual void write(star::StarBuffer& buffer) = 0;

        virtual bool isValid(const uint8_t& currentFrameInFlightIndex) const {
            if(frameInFlightIndexToUpdateOn.has_value() && currentFrameInFlightIndex == *frameInFlightIndexToUpdateOn) {
                return true;
            }

            return false;
        }

        const std::optional<uint8_t>& getFrameInFlightIndexToUpdateOn() const { return frameInFlightIndexToUpdateOn; }

        const BufferCreationArgs& getCreationArgs() const { return creationArgs; }

        //If buffer needs to be recreated, should give a handle with the request
        //BufferManagerRequest(handle handle)

protected:
        BufferCreationArgs creationArgs;
        std::optional<uint8_t> frameInFlightIndexToUpdateOn = std::nullopt;

private:

    };
}