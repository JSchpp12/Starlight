#pragma once

#include "StarBuffer.hpp"
#include "ManagerController.hpp"
#include "TextureMemoryTransferRequest.hpp"

#include <vulkan/vulkan.hpp>

namespace star{
    class TextureManagerRequest : public ManagerController<TextureMemoryTransferRequest>{
        public:
        TextureManagerRequest() = default; 
        TextureManagerRequest(const uint8_t& frameInFlightIndexToUpdateOn) : ManagerController(frameInFlightIndexToUpdateOn){}

        virtual std::unique_ptr<TextureMemoryTransferRequest> createTransferRequest() const = 0;
    };
}