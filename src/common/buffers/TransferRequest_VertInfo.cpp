#include "TransferRequest_VertInfo.hpp"

#include "CastHelpers.hpp"

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::VertInfo::createFinal(
    vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    std::vector<uint32_t> indices = {this->graphicsQueueIndex};
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
                .setSize(sizeof(Vertex) * vertices.size())
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer),
            "VertexBuffer")
        .setInstanceCount(vertices.size())
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
                .setSize(sizeof(Vertex) * vertices.size())
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "VertexBuffer_Stage")
        .setInstanceCount(vertices.size())
        .setInstanceSize(sizeof(Vertex))
        .build();
}

void star::TransferRequest::VertInfo::writeDataToStageBuffer(StarBuffers::Buffer &buffer) const
{
    void *mapped = nullptr;
    buffer.map(&mapped);

    int numVerts = 0; 
    CastHelpers::SafeCast<size_t, int>(vertices.size(), numVerts); 

    for (int i = 0; i < numVerts; i++)
    {
        Vertex vert = Vertex(vertices[i]); 

        buffer.writeToIndex(&vert, mapped, i);
    }

    buffer.unmap();
}