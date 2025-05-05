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
    
        StarBuffer::BufferCreationArgs getCreateArgs() const override{
            return StarBuffer::BufferCreationArgs{
                sizeof(glm::mat4),
                static_cast<uint32_t>(this->displayMatrixInfo.size()),
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                VMA_MEMORY_USAGE_AUTO,
                vk::BufferUsageFlagBits::eUniformBuffer,
                vk::SharingMode::eConcurrent,
                "InstanceModelInfoBuffer"
            };
        }
        
        void writeData(StarBuffer& buffer) const override;
    
        protected:
        std::vector<glm::mat4> displayMatrixInfo;
    };
}