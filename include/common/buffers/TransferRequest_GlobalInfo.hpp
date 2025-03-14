#pragma once 

#include "TransferRequest_Memory.hpp"
#include "StarCamera.hpp"

#include <glm/glm.hpp>

namespace star::TransferRequest{
    class GlobalInfo : public Memory<star::StarBuffer::BufferCreationArgs>{
        public:
        
        GlobalInfo(const StarCamera& camera, const int& numLights) : camera(camera), numLights(numLights){}

        StarBuffer::BufferCreationArgs getCreateArgs(const vk::PhysicalDeviceProperties& deviceProperties) const override{
            return StarBuffer::BufferCreationArgs{
                sizeof(GlobalUniformBufferObject),
                1,
                (VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT), 
                VMA_MEMORY_USAGE_AUTO, 
                vk::BufferUsageFlagBits::eUniformBuffer, 
                vk::SharingMode::eConcurrent, 
                "GlobalInfoBuffer"
            };
        }

        void writeData(StarBuffer& buffer) const override; 

        private:
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