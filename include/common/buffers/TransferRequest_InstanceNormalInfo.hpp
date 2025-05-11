#pragma once

#include "TransferRequest_Buffer.hpp"
#include "StarObjectInstance.hpp"

#include <glm/glm.hpp>

namespace star::TransferRequest{
    class InstanceNormalInfo : public Buffer{
        public:
        InstanceNormalInfo(const std::vector<std::unique_ptr<StarObjectInstance>>& objectInstances) {
            for (auto& instance : objectInstances) {
                this->normalMatrixInfo.push_back(instance->getDisplayMatrix());
            }
        }

        std::unique_ptr<StarBuffer> createStagingBuffer(vk::Device& device, VmaAllocator& allocator, const uint32_t& transferQueueFamilyIndex) const override; 

        std::unique_ptr<StarBuffer> createFinal(vk::Device &device, VmaAllocator &allocator, const uint32_t& transferQueueFamilyIndex) const override; 
        
        void writeDataToStageBuffer(StarBuffer& buffer) const override; 

        protected:
            std::vector<glm::mat4> normalMatrixInfo = std::vector<glm::mat4>(); 


    };
}