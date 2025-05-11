#include "TransferRequest_GlobalInfo.hpp"

std::unique_ptr<star::StarBuffer> star::TransferRequest::GlobalInfo::createStagingBuffer(vk::Device& device, VmaAllocator& allocator, const uint32_t& transferQueueFamilyIndex) const{
    auto createArgs = StarBuffer::BufferCreationArgs{
        sizeof(GlobalUniformBufferObject),
        1,
        (VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT), 
        VMA_MEMORY_USAGE_AUTO, 
        vk::BufferUsageFlagBits::eTransferSrc, 
        vk::SharingMode::eConcurrent, 
        "GlobalInfoBuffer"
    };

    return std::make_unique<StarBuffer>(allocator, createArgs); 
}

std::unique_ptr<star::StarBuffer> star::TransferRequest::GlobalInfo::createFinal(vk::Device &device, VmaAllocator &allocator, const uint32_t& transferQueueFamilyIndex) const{
    auto createArgs = StarBuffer::BufferCreationArgs{
        sizeof(GlobalUniformBufferObject),
        1,
        (VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT), 
        VMA_MEMORY_USAGE_AUTO, 
        vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst, 
        vk::SharingMode::eConcurrent, 
        "GlobalInfoBuffer"
    };

    return std::make_unique<StarBuffer>(allocator, createArgs); 
}

void star::TransferRequest::GlobalInfo::writeDataToStageBuffer(star::StarBuffer& buffer) const{
    buffer.map(); 

    //update global ubo 
    GlobalUniformBufferObject globalUbo;
    globalUbo.proj = this->camera.getProjectionMatrix();
    //glm designed for openGL where the Y coordinate of the flip coordinates is inverted. Fix this by flipping the sign on the scaling factor of the Y axis in the projection matrix.
    globalUbo.proj[1][1] *= -1;
    globalUbo.view = this->camera.getViewMatrix();
    globalUbo.inverseView = glm::inverse(this->camera.getViewMatrix());
    globalUbo.numLights = static_cast<uint32_t>(this->numLights);

    buffer.writeToBuffer(&globalUbo, sizeof(globalUbo)); 

    buffer.unmap(); 
}