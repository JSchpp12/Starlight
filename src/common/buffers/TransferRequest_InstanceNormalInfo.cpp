#include "TransferRequest_InstanceNormalInfo.hpp"

#include "CastHelpers.hpp"

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::InstanceNormalInfo::createStagingBuffer(
    vk::Device &device, VmaAllocator &allocator) const
{
    const vk::DeviceSize alignmentSize =
        StarBuffers::Buffer::GetAlignment(sizeof(glm::mat4), this->minUniformBufferOffsetAlignment);

    return StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eExclusive)
                .setSize(CastHelpers::size_t_to_unsigned_int(this->normalMatrixInfo.size() * alignmentSize))
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "InstanceNormalInfo_SRC")
        .setInstanceCount(CastHelpers::size_t_to_unsigned_int(this->normalMatrixInfo.size()))
        .setInstanceSize(sizeof(glm::mat4))
        .setMinOffsetAlignment(this->minUniformBufferOffsetAlignment)
        .build();
}

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::InstanceNormalInfo::createFinal(
    vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    std::vector<uint32_t> indices = {this->graphicsQueueFamilyIndex};
	for (const auto &queueFamilyIndex : transferQueueFamilyIndex)
		indices.push_back(queueFamilyIndex);

    const vk::DeviceSize alignmentSize =
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
                .setSize(CastHelpers::size_t_to_unsigned_int(this->normalMatrixInfo.size() * alignmentSize))
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer),
            "InstanceNormalInfo")
        .setInstanceCount(CastHelpers::size_t_to_unsigned_int(this->normalMatrixInfo.size()))
        .setInstanceSize(sizeof(glm::mat4))
        .setMinOffsetAlignment(this->minUniformBufferOffsetAlignment)
        .build();
}

void star::TransferRequest::InstanceNormalInfo::writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const
{
    buffer.map();

    std::vector<glm::mat4> inverseTranspose = std::vector<glm::mat4>(this->normalMatrixInfo.size());

    for (int i = 0; i < this->normalMatrixInfo.size(); i++)
    {
        inverseTranspose[i] = glm::inverse(glm::transpose(this->normalMatrixInfo[i]));
    }

    buffer.writeToBuffer(inverseTranspose.data());
    buffer.unmap();
}