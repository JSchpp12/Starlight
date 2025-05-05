#pragma once 

#include "ManagerController_RenderResource_Buffer.hpp"

namespace star::ManagerController::RenderResource{
    class IndicesInfo : public Buffer{
        public:
        IndicesInfo(const std::vector<uint32_t>& indices) : indices(indices){};

        std::vector<std::unique_ptr<TransferRequest::Memory<StarBuffer::BufferCreationArgs>>> createTransferRequests(const vk::PhysicalDevice& physicalDevice) override;

        protected:
        const std::vector<uint32_t> indices;
    };
}