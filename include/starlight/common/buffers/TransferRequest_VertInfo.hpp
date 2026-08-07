#pragma once

#include "TransferRequest_Buffer.hpp"
#include "Vertex.hpp"

#include <star_common/helper/CastHelpers.hpp>

#include <stdexcept>
#include <vector>

namespace star::TransferRequest
{
/// Vertex buffer upload request. Templated on the host vertex type so that
/// compact, application-specific vertex formats (e.g. a terrain vertex with
/// only pos/normal/texCoord) can be uploaded without the full engine Vertex.
/// Defaults to the engine's Vertex so existing call sites behave unchanged.
template <typename T = Vertex>
class VertInfo : public Buffer
{
  public:
    VertInfo(const uint32_t &graphicsQueueIndex, std::vector<T> vertices)
        : graphicsQueueIndex(graphicsQueueIndex), vertices(std::move(vertices))
    {
    }

    std::unique_ptr<StarBuffers::Buffer> createFinal(vk::Device &device, VmaAllocator &allocator,
                                                     const std::vector<uint32_t> &transferQueueFamilyIndex) const override
    {
        std::vector<uint32_t> indices = {this->graphicsQueueIndex};
        for (const auto &index : transferQueueFamilyIndex)
            indices.push_back(index);

        uint32_t numVerts = 0, numInds = 0;
        if (!star::common::casts::SafeCast<size_t, uint32_t>(vertices.size(), numVerts) ||
            !star::common::casts::SafeCast<size_t, uint32_t>(indices.size(), numInds))
        {
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
                    .setSize(sizeof(T) * numVerts)
                    .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer),
                "VertexBuffer")
            .setInstanceCount(numVerts)
            .setInstanceSize(sizeof(T))
            .buildUnique();
    }

    std::unique_ptr<StarBuffers::Buffer> createStagingBuffer(vk::Device &device, VmaAllocator &allocator) const override
    {
        uint32_t numVerts = 0;
        if (!star::common::casts::SafeCast<size_t, uint32_t>(this->vertices.size(), numVerts))
        {
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
                    .setSize(sizeof(T) * numVerts)
                    .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
                "VertexBuffer_Stage")
            .setInstanceCount(numVerts)
            .setInstanceSize(sizeof(T))
            .buildUnique();
    }

    void writeDataToStageBuffer(StarBuffers::Buffer &buffer) const override
    {
        void *mapped = nullptr;
        buffer.map(&mapped);

        for (size_t i{0}; i < vertices.size(); i++)
        {
            T vert = T(vertices[i]);

            buffer.writeToIndex(&vert, mapped, i);
        }

        buffer.unmap();
    }

  protected:
    const uint32_t graphicsQueueIndex;
    std::vector<T> vertices;
};
} // namespace star::TransferRequest