#include "TransferRequest_LightInfo.hpp"

#include "starlight/core/Exceptions.hpp"
#include "starlight/maker/buffer/MakeBuffer.hpp"

#include <star_common/helper/CastHelpers.hpp>

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::LightInfo::createStagingBuffer(
    core::device::StarDevice &device) const
{
    star::StarBuffers::Buffer builtBuffer;
    const auto makeResult = star::maker::buffer::MakeBuffer(device)
                                .setCreateInfo(Allocator::AllocationBuilder()
                                                   .setFlags(VMA_ALLOCATION_CREATE_MAPPED_BIT |
                                                             VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
                                                   .setUsage(VMA_MEMORY_USAGE_AUTO)
                                                   .build(),
                                               vk::BufferCreateInfo()
                                                   .setSharingMode(vk::SharingMode::eExclusive)
                                                   .setSize(sizeof(int))
                                                   .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
                                               "LightInfo_Stage")
                                .setInstanceCount(star::common::casts::size_t_to_unsigned_int(1))
                                .setInstanceSize(sizeof(int))
                                .make(&builtBuffer, 1);
    if (!makeResult)
    {
        STAR_THROW("Failed to create staging buffer for light info: " + makeResult.error().detail);
    }

    return std::make_unique<star::StarBuffers::Buffer>(std::move(builtBuffer));
}

std::unique_ptr<star::StarBuffers::Buffer> star::TransferRequest::LightInfo::createFinal(
    core::device::StarDevice &device, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    std::vector<uint32_t> indices{this->graphicsQueueFamilyIndex};
    indices.reserve(transferQueueFamilyIndex.size() + 1);

    for (auto &index : transferQueueFamilyIndex)
        indices.push_back(index);

    star::StarBuffers::Buffer builtBuffer;
    const auto makeResult = star::maker::buffer::MakeBuffer(device)
                                .setCreateInfo(Allocator::AllocationBuilder()
                                                   .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                                                   .setUsage(VMA_MEMORY_USAGE_AUTO)
                                                   .build(),
                                               vk::BufferCreateInfo()
                                                   .setSharingMode(vk::SharingMode::eExclusive)
                                                   .setSize(sizeof(int))
                                                   .setUsage(vk::BufferUsageFlagBits::eTransferDst |
                                                             vk::BufferUsageFlagBits::eUniformBuffer),
                                               "LightInfo")
                                .setInstanceCount(1)
                                .setInstanceSize(sizeof(int))
                                .make(&builtBuffer, 1);
    if (!makeResult)
    {
        STAR_THROW("Failed to create final buffer for light info: " + makeResult.error().detail);
    }

    return std::make_unique<star::StarBuffers::Buffer>(std::move(builtBuffer));
}

void star::TransferRequest::LightInfo::writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const
{
    void *mapped = nullptr;
    buffer.map(&mapped);

    uint32_t num;
    star::common::casts::SafeCast<size_t, uint32_t>(m_numLights, num);

    Info info = Info{.numLights = num};

    buffer.writeToBuffer(&info, mapped, sizeof(int));
    buffer.unmap();
}