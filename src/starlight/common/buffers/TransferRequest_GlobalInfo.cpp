#include "TransferRequest_GlobalInfo.hpp"

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::GlobalInfo::createStagingBuffer(
    vk::Device &device, VmaAllocator &allocator) const
{
    return StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eExclusive)
                .setSize(sizeof(GlobalUniformBufferObject))
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "GlobalInfo_TransferSRC")
        .setInstanceCount(1)
        .setInstanceSize(sizeof(GlobalUniformBufferObject))
        .buildUnique();
}

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::GlobalInfo::createFinal(
    vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    uint32_t indices[3];
    uint8_t count{0};

    assert(m_queueFamilyIndices.size() < 3 && "Current array implementation only supports 3 unique total families");

    for (size_t i{0}; i < m_queueFamilyIndices.size(); i++)
    {
        indices[i] = m_queueFamilyIndices[i];
        count++;
    }

    indices[count] = transferQueueFamilyIndex.front();
    count++;

    const auto bCreate = vk::BufferCreateInfo()
                             .setSharingMode(vk::SharingMode::eConcurrent)
                             .setSize(sizeof(GlobalUniformBufferObject))
                             .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer)
                             .setQueueFamilyIndexCount(count)
                             .setPQueueFamilyIndices(indices);
    return StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(Allocator::AllocationBuilder()
                                     .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                                     .setUsage(VMA_MEMORY_USAGE_AUTO)
                                     .build(),
                                 bCreate, "GlobalInfo")
        .setInstanceCount(1)
        .setInstanceSize(sizeof(GlobalUniformBufferObject))
        .buildUnique();
}

void star::TransferRequest::GlobalInfo::writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const
{
    void *mapped = nullptr;
    buffer.map(&mapped);

    // update global ubo
    GlobalUniformBufferObject globalUbo;
    globalUbo.proj = this->camera.getProjectionMatrix();
    // glm designed for openGL where the Y coordinate of the flip coordinates is inverted. Fix this by flipping the sign
    // on the scaling factor of the Y axis in the projection matrix.
    globalUbo.view = this->camera.getViewMatrix();
    globalUbo.inverseView = glm::inverse(this->camera.getViewMatrix());

    buffer.writeToBuffer(&globalUbo, mapped, sizeof(globalUbo));

    buffer.unmap();
}