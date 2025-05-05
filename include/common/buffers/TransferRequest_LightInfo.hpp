#pragma once

#include "TransferRequest_Buffer.hpp"
#include "Light.hpp"

#include <glm/glm.hpp>
#include <vector>

namespace star::TransferRequest{
    class LightInfo : public Buffer{
        public:
        struct LightBufferObject {
            glm::vec4 position = glm::vec4(1.0f);
            glm::vec4 direction = glm::vec4(1.0f);     //direction in which the light is pointing
            glm::vec4 ambient = glm::vec4(1.0f);
            glm::vec4 diffuse = glm::vec4(1.0f);
            glm::vec4 specular = glm::vec4(1.0f);
            //controls.x = inner cutoff diameter 
            //controls.y = outer cutoff diameter
            glm::vec4 controls = glm::vec4(0.0f);       //container for single float values
            //settings.x = enabled
            //settings.y = type
            glm::uvec4 settings = glm::uvec4(0);    //container for single uint values
        };
    
        LightInfo(const std::vector<std::unique_ptr<Light>>& lights)
        {
            for (int i = 0; i < lights.size(); ++i)
            {
                myLights.push_back(Light(*lights[i].get()));
            } 
        }

        void writeData(StarBuffer& buffer) const override;
    
        StarBuffer::BufferCreationArgs getCreateArgs() const override{
            return StarBuffer::BufferCreationArgs{
                sizeof(LightBufferObject),
                static_cast<uint32_t>(this->myLights.size()),
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                VMA_MEMORY_USAGE_AUTO,
                vk::BufferUsageFlagBits::eStorageBuffer,
                vk::SharingMode::eConcurrent,
                "LightInfoBuffer"
            };
        }
    
        protected:
        std::vector<Light> myLights; 
    
    };
}