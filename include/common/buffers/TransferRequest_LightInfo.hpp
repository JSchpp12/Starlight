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

        std::unique_ptr<StarBuffer> createStagingBuffer(vk::Device& device, VmaAllocator& allocator) const override; 

        std::unique_ptr<StarBuffer> createFinal(vk::Device &device, VmaAllocator &allocator) const override; 
        
        void writeDataToStageBuffer(StarBuffer& buffer) const override; 
    
        protected:
        std::vector<Light> myLights; 
    
    };
}