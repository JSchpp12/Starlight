#include "TransferRequest_Buffer.hpp"

void star::TransferRequest::Buffer::copyFromTransferSRCToDST(StarBuffers::Buffer &srcBuffer, StarBuffers::Buffer &dstBuffer,
                                                             vk::CommandBuffer &commandBuffer) const
{
    DefaultCopy(srcBuffer, dstBuffer, commandBuffer);
}

void star::TransferRequest::Buffer::DefaultCopy(StarBuffers::Buffer &srcBuffer, StarBuffers::Buffer &dstBuffer,
                                                vk::CommandBuffer &commandBuffer)
{
    vk::BufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = srcBuffer.getBufferSize();

    commandBuffer.copyBuffer(srcBuffer.getVulkanBuffer(), dstBuffer.getVulkanBuffer(), copyRegion);
}