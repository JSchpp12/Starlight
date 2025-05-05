#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "VulkanVertex.hpp"

namespace star::ManagerController::RenderResource{
    class VertInfo : public ManagerController::RenderResource::Buffer{
        public:
        VertInfo(const std::vector<Vertex> vertices) : vertices(vertices){}

        std::vector<std::unique_ptr<TransferRequest::Memory<StarBuffer::BufferCreationArgs>>> createTransferRequests(const vk::PhysicalDevice& physicalDevice) override;

        protected:
        const std::vector<Vertex> vertices;
    };
}