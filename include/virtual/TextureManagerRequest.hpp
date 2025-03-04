#pragma once

#include "StarBuffer.hpp"
#include "ManagerController.hpp"

#include <vulkan/vulkan.hpp>

namespace star{
    class TextureManagerRequest : public ManagerController{
        public:
        TextureManagerRequest() = default; 
        TextureManagerRequest(const uint8_t& frameInFlightIndexToUpdateOn) : ManagerController(frameInFlightIndexToUpdateOn){}
        protected:

        private:

    };
}