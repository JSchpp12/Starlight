#pragma once

#include "TransferRequest_Buffer.hpp"
#include "StarBuffer.hpp"
#include "StarObjectInstance.hpp"

#include <glm/glm.hpp>

namespace star::TransferRequest{
    class InstanceModelInfo : public star::TransferRequest::Buffer{
        public:
        InstanceModelInfo(const std::vector<std::unique_ptr<star::StarObjectInstance>>& objectInstances)
            : displayMatrixInfo(std::vector<glm::mat4>(objectInstances.size()))
            {
                for (int i = 0; i < objectInstances.size(); i++){
                    displayMatrixInfo[i] = objectInstances[i]->getDisplayMatrix();
                }
            }

        std::unique_ptr<StarBuffer> createStagingBuffer(vk::Device& device, VmaAllocator& allocator) const override; 

        std::unique_ptr<StarBuffer> createFinal(vk::Device &device, VmaAllocator &allocator) const override; 
        
        void writeDataToStageBuffer(StarBuffer& buffer) const override; 
    
        protected:
        std::vector<glm::mat4> displayMatrixInfo;
    };
}