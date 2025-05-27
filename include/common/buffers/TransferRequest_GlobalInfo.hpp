#pragma once 

#include "TransferRequest_Buffer.hpp"
#include "StarCamera.hpp"

#include <glm/glm.hpp>

namespace star::TransferRequest{
    class GlobalInfo : public Buffer{
        public:
        
        GlobalInfo(const StarCamera& camera, 
            const int& numLights, 
            const uint32_t &graphicsQueueIndex, 
        const vk::DeviceSize &minUniformBufferOffsetAlignment) 
        : camera(camera), 
        numLights(numLights), 
        graphicsQueueIndex(graphicsQueueIndex),
        minUniformBufferOffsetAlignment(minUniformBufferOffsetAlignment){}

        std::unique_ptr<StarBuffer> createStagingBuffer(vk::Device& device, VmaAllocator& allocator, const uint32_t& transferQueueFamilyIndex) const override; 

        std::unique_ptr<StarBuffer> createFinal(vk::Device &device, VmaAllocator &allocator, const uint32_t& transferQueueFamilyIndex) const override; 
        
        void writeDataToStageBuffer(StarBuffer& buffer) const override; 

        private:
        const vk::DeviceSize minUniformBufferOffsetAlignment;
        const uint32_t graphicsQueueIndex; 
        const int numLights = 0;
        const StarCamera camera; 

        struct GlobalUniformBufferObject {
            alignas(16) glm::mat4 proj;
            alignas(16) glm::mat4 view;
            alignas(16) glm::mat4 inverseView;              //used to extrapolate camera position, can be used to convert from camera to world space
            uint32_t numLights;                             //number of lights in render
        };
    };
}