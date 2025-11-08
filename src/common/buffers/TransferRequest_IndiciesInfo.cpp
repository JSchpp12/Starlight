#include "TransferRequest_IndicesInfo.hpp"

#include "CastHelpers.hpp"

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::IndicesInfo::createStagingBuffer(vk::Device &device,
                                                                                          VmaAllocator &allocator) const
{
    uint32_t numIndices = 0; 
    CastHelpers::SafeCast<size_t, uint32_t>(this->indices.size(), numIndices); 

    return StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eExclusive)
                .setSize(sizeof(uint32_t) * CastHelpers::size_t_to_unsigned_int(this->indices.size()))
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "IndicesInfoBuffer_Src")
        .setInstanceCount(CastHelpers::size_t_to_unsigned_int(this->indices.size()))
        .setInstanceSize(sizeof(uint32_t))
        .buildUnique();
}

void star::TransferRequest::IndicesInfo::writeDataToStageBuffer(StarBuffers::Buffer &buffer) const
{
    void *mapped = nullptr;
    buffer.map(&mapped);

    vk::DeviceSize indSize = sizeof(uint32_t) * this->indices.size();
    std::vector<uint32_t> cpInd{this->indices};
    buffer.writeToBuffer(cpInd.data(), mapped, indSize);

    buffer.unmap();
}

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::IndicesInfo::createFinal(
    vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    std::vector<uint32_t> indices = {this->graphicsQueueFamilyIndex};
    for (const auto &index : transferQueueFamilyIndex)
        indices.push_back(index);

    uint32_t numIndices = 0; 
    CastHelpers::SafeCast<size_t, uint32_t>(indices.size(), numIndices); 

    return StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eConcurrent)
                .setQueueFamilyIndexCount(indices.size())
                .setPQueueFamilyIndices(indices.data())
                .setSize(sizeof(uint32_t) * CastHelpers::size_t_to_unsigned_int(this->indices.size()))
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer),
            "IndicesInfoBuffer_Src")
        .setInstanceCount(CastHelpers::size_t_to_unsigned_int(this->indices.size()))
        .setInstanceSize(sizeof(uint32_t))
        .buildUnique();
}
