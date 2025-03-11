#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "TransferRequest_Memory.hpp"
#include "CastHelpers.hpp"
#include "StarBuffer.hpp"

#include <vector>

namespace star{
    namespace TransferRequest{
        class ObjIndicies : public Memory<StarBuffer::BufferCreationArgs> {
            public:
            ObjIndicies(const std::vector<uint32_t>& indices) : indices(indices){}
    
            void writeData(StarBuffer& buffer) const override;
    
            StarBuffer::BufferCreationArgs getCreateArgs(const vk::PhysicalDeviceProperties& deviceProperties) const override{
                return StarBuffer::BufferCreationArgs{
                    sizeof(uint32_t),
                    CastHelpers::size_t_to_unsigned_int(this->indices.size()),
                    VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
                    VMA_MEMORY_USAGE_AUTO, 
                    vk::BufferUsageFlagBits::eIndexBuffer, 
                    vk::SharingMode::eConcurrent
                };
            }
    
            protected:
            const std::vector<uint32_t> indices;
        };
    }

    class ObjIndicesInfo : public ManagerController::RenderResource::Buffer{
        public:
        ObjIndicesInfo(const std::vector<uint32_t>& indices) : indices(indices){};

        std::unique_ptr<TransferRequest::Memory<StarBuffer::BufferCreationArgs>> createTransferRequest() const override;

        protected:
        const std::vector<uint32_t> indices;
    };
}