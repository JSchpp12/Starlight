#include "TransferRequest_VertInfo.hpp"

#include "CastHelpers.hpp"

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::VertInfo::createFinal(
    vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    std::vector<uint32_t> indices = {this->graphicsQueueIndex};
    for (const auto &index : transferQueueFamilyIndex)
        indices.push_back(index);

    uint32_t numIndices = 0;
    CastHelpers::SafeCast<size_t, uint32_t>(indices.size(), numIndices); 

    uint32_t numVerts = 0; 
    CastHelpers::SafeCast<size_t, uint32_t>(vertices.size(), numVerts); 

    return StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eConcurrent)
                .setPQueueFamilyIndices(indices.data())
                .setQueueFamilyIndexCount(numIndices)
                .setSize(sizeof(Vertex) * numVerts)
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer),
            "VertexBuffer")
        .setInstanceCount(numVerts)
        .setInstanceSize(sizeof(Vertex))
        .build();
}

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::VertInfo::createStagingBuffer(
    vk::Device &device, VmaAllocator &allocator) const
{
    uint32_t numVerts = 0; 
    CastHelpers::SafeCast<size_t, uint32_t>(this->vertices.size(), numVerts); 

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
        .build();
}

void star::TransferRequest::VertInfo::writeDataToStageBuffer(StarBuffers::Buffer &buffer) const
{
    void *mapped = nullptr;
    buffer.map(&mapped);

    auto data = this->getVertices();

    for (size_t i = 0; i < data.size(); ++i)
    {
        int index = 0;
        CastHelpers::SafeCast<size_t, int>(i, index); 
        buffer.writeToIndex(&data.at(i), mapped, index);
    }

    buffer.unmap();
}

std::vector<star::Vertex> star::TransferRequest::VertInfo::getVertices() const
{
    return this->vertices;
}
