#include "TransferRequest_InstanceModelInfo.hpp"

#include "CastHelpers.hpp"

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::InstanceModelInfo::createStagingBuffer(
    vk::Device &device, VmaAllocator &allocator) const
{
    const vk::DeviceSize alignmentInstanceSize =
        StarBuffers::Buffer::GetAlignment(sizeof(glm::mat4), this->minUniformBufferOffsetAlignment);

    return StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eExclusive)
                .setSize(CastHelpers::size_t_to_unsigned_int(this->displayMatrixInfo.size() * alignmentInstanceSize))
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "InstanceModelInfo_Src")
        .setInstanceCount(CastHelpers::size_t_to_unsigned_int(this->displayMatrixInfo.size()))
        .setInstanceSize(sizeof(glm::mat4))
        .setMinOffsetAlignment(this->minUniformBufferOffsetAlignment)
        .build();
}

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::InstanceModelInfo::createFinal(
    vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    std::vector<uint32_t> indices = {this->graphicsQueueFamilyIndex};
	for (auto &index : transferQueueFamilyIndex)
		indices.push_back(index);

    const vk::DeviceSize alignmentInstanceSize =
        StarBuffers::Buffer::GetAlignment(sizeof(glm::mat4), this->minUniformBufferOffsetAlignment);

    return StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eConcurrent)
                .setQueueFamilyIndexCount(2)
                .setQueueFamilyIndices(indices)
                .setSize(CastHelpers::size_t_to_unsigned_int(this->displayMatrixInfo.size() * alignmentInstanceSize))
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer),
            "InstanceModelInfo")
        .setInstanceCount(CastHelpers::size_t_to_unsigned_int(this->displayMatrixInfo.size()))
        .setInstanceSize(sizeof(glm::mat4))
        .setMinOffsetAlignment(this->minUniformBufferOffsetAlignment)
        .build();
}

void star::TransferRequest::InstanceModelInfo::writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const
{
    void *mapped = nullptr;
    buffer.map(&mapped);

    for (size_t i = 0; i < this->displayMatrixInfo.size(); ++i)
    {
        glm::mat4 info = glm::mat4(this->displayMatrixInfo[i]);
        buffer.writeToIndex(&info, mapped, i);
    }

    buffer.unmap();
}