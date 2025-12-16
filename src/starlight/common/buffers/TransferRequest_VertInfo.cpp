#include "TransferRequest_VertInfo.hpp"

#include <star_common/helper/CastHelpers.hpp>

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::VertInfo::createFinal(
    vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    std::vector<uint32_t> indices = {this->graphicsQueueIndex};
    for (const auto &index : transferQueueFamilyIndex)
        indices.push_back(index);

    uint32_t numVerts = 0, numInds = 0;
    if (!common::helper::SafeCast<size_t, uint32_t>(vertices.size(), numVerts) || !common::helper::SafeCast<size_t, uint32_t>(indices.size(), numInds)){
        throw std::runtime_error("Failed to parse numerical values for vert info buffer creation"); 
    } 

    return StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eConcurrent)
                .setPQueueFamilyIndices(indices.data())
                .setQueueFamilyIndexCount(numInds)
                .setSize(sizeof(Vertex) * numVerts)
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer),
            "VertexBuffer")
        .setInstanceCount(numVerts)
        .setInstanceSize(sizeof(Vertex))
        .buildUnique();
}

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::VertInfo::createStagingBuffer(
    vk::Device &device, VmaAllocator &allocator) const
{
    uint32_t numVerts = 0; 
    if (!common::helper::SafeCast<size_t, uint32_t>(this->vertices.size(), numVerts)){
        throw std::runtime_error("Failed to cast numerical info for vert info creation"); 
    } 

    return StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eExclusive)
                .setSize(sizeof(Vertex) * numVerts)
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "VertexBuffer_Stage")
        .setInstanceCount(numVerts)
        .setInstanceSize(sizeof(Vertex))
        .buildUnique();
}

void star::TransferRequest::VertInfo::writeDataToStageBuffer(StarBuffers::Buffer &buffer) const
{
    void *mapped = nullptr;
    buffer.map(&mapped);

    int numVerts = 0; 
    common::helper::SafeCast<size_t, int>(vertices.size(), numVerts); 

    for (int i = 0; i < numVerts; i++)
    {
        Vertex vert = Vertex(vertices[i]); 

        buffer.writeToIndex(&vert, mapped, i);
    }

    buffer.unmap();
}