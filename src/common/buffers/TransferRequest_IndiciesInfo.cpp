#include "TransferRequest_IndicesInfo.hpp"

#include "CastHelpers.hpp"

std::unique_ptr<star::StarBuffer> star::TransferRequest::IndicesInfo::createStagingBuffer(vk::Device& device, VmaAllocator& allocator, const uint32_t& transferQueueFamilyIndex) const{
    return StarBuffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eExclusive)
                .setSize(sizeof(uint32_t) * CastHelpers::size_t_to_unsigned_int(this->indices.size()))
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "IndicesInfoBuffer_Src"
        )
        .setInstanceCount(CastHelpers::size_t_to_unsigned_int(this->indices.size()))
        .setInstanceSize(sizeof(uint32_t))
        .build();
}

void star::TransferRequest::IndicesInfo::writeDataToStageBuffer(StarBuffer& buffer) const{
    buffer.map();
    
    vk::DeviceSize indSize = sizeof(uint32_t) * this->indices.size();
    std::vector<uint32_t> cpInd{this->indices};
    buffer.writeToBuffer(cpInd.data(), indSize);

    buffer.unmap();
}

std::unique_ptr<star::StarBuffer> star::TransferRequest::IndicesInfo::createFinal(vk::Device &device, VmaAllocator &allocator, const uint32_t& transferQueueFamilyIndex) const{
    std::vector<uint32_t> indices = {
        this->graphicsQueueFamilyIndex, 
        transferQueueFamilyIndex
    };

    return StarBuffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eConcurrent)
                .setQueueFamilyIndexCount(2)
                .setQueueFamilyIndices(indices)
                .setSize(sizeof(uint32_t) * CastHelpers::size_t_to_unsigned_int(this->indices.size()))
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer),
            "IndicesInfoBuffer_Src"
        )
        .setInstanceCount(CastHelpers::size_t_to_unsigned_int(this->indices.size()))
        .setInstanceSize(sizeof(uint32_t))
        .build();
}
