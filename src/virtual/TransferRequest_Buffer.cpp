#include "TransferRequest_Buffer.hpp"

void star::TransferRequest::Buffer::copyFromTransferSRCToDST(StarBuffer& srcBuffer, StarBuffer& dstBuffer, vk::CommandBuffer& commandBuffer) const{
    defaultCopy(srcBuffer, dstBuffer, commandBuffer);
}

void star::TransferRequest::Buffer::defaultCopy(StarBuffer& srcBuffer, StarBuffer& dstBuffer, vk::CommandBuffer& commandBuffer){
    vk::BufferCopy copyRegion{}; 
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = srcBuffer.getBufferSize(); 

    commandBuffer.copyBuffer(srcBuffer.getVulkanBuffer(), dstBuffer.getVulkanBuffer(), copyRegion);
}