#include "TransferRequest_LightInfo.hpp"

#include "CastHelpers.hpp"

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::LightInfo::createStagingBuffer(
    vk::Device &device, VmaAllocator &allocator) const
{
    return StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eExclusive)
                .setSize(sizeof(int))
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "LightList_Stage")
        .setInstanceCount(CastHelpers::size_t_to_unsigned_int(1))
        .setInstanceSize(sizeof(int))
        .build();
}

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::LightInfo::createFinal(
    vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    std::vector<uint32_t> indices = std::vector<uint32_t>{this->graphicsQueueFamilyIndex};
    for (const auto &index : transferQueueFamilyIndex)
        indices.push_back(index);

    return StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eConcurrent)
                .setPQueueFamilyIndices(indices.data())
                .setQueueFamilyIndexCount(indices.size())
                .setSize(sizeof(int))
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer),
            "LightInfo")
        .setInstanceCount(1)
        .setInstanceSize(sizeof(int))
        .build();
}

void star::TransferRequest::LightInfo::writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const
{
    void *mapped = nullptr;
    buffer.map(&mapped);

    uint32_t num; 
    star::CastHelpers::SafeCast<size_t, uint32_t>(m_numLights, num); 

    Info info = Info{
        .numLights = num
    }; 

    buffer.writeToBuffer(&info, mapped, sizeof(int));
    buffer.unmap();  
}