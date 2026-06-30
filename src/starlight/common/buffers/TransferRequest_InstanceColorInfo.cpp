#include "starlight/common/buffers/TransferRequest_InstanceColorInfo.hpp"

#include "starlight/core/Exceptions.hpp"

#include <star_common/helper/CastHelpers.hpp>

namespace star::TransferRequest
{
InstanceColorInfo::InstanceColorInfo(std::vector<star::Color> colors, uint32_t graphicsQueueFamilyIndex)
    : m_colors(std::move(colors)), m_graphicsQueueFamilyIndex(std::move(graphicsQueueFamilyIndex))
{
}

static vk::DeviceSize GetMemorySize(const std::vector<Color> colors)
{
    return sizeof(glm::vec4) * colors.size();
}

std::unique_ptr<StarBuffers::Buffer> star::TransferRequest::InstanceColorInfo::createStagingBuffer(
    vk::Device &device, VmaAllocator &allocator) const
{
    const vk::DeviceSize bSize = GetMemorySize(m_colors);

    return StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eExclusive)
                .setSize(bSize)
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "InstanceColorInfo_src")
        .setInstanceCount(1)
        .setInstanceSize(bSize)
        .buildUnique();
}

std::unique_ptr<StarBuffers::Buffer> star::TransferRequest::InstanceColorInfo::createFinal(
    vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    const vk::DeviceSize bSize = GetMemorySize(m_colors);

    std::vector<uint32_t> indices{m_graphicsQueueFamilyIndex};
    indices.reserve(transferQueueFamilyIndex.size() + 1);

    for (auto &index : transferQueueFamilyIndex)
        indices.push_back(index);

    return StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eConcurrent)
                .setQueueFamilyIndexCount(static_cast<uint32_t>(indices.size()))
                .setQueueFamilyIndices(indices)
                .setSize(bSize)
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer),
            "InstanceModelInfo")
        .setInstanceCount(1)
        .setInstanceSize(bSize)
        .buildUnique();
}

void star::TransferRequest::InstanceColorInfo::writeDataToStageBuffer(StarBuffers::Buffer &buffer) const
{
    void *mapped{nullptr};
    buffer.map(&mapped);

    std::vector<glm::vec4> colorData;
    colorData.reserve(m_colors.size());
    for (size_t i{0}; i < m_colors.size(); i++)
        colorData.emplace_back(m_colors[i].getR(), m_colors[i].getG(), m_colors[i].getB(), 1.0f);

    buffer.writeToBuffer(colorData.data(), mapped, GetMemorySize(m_colors));
    buffer.unmap();
}

} // namespace star::TransferRequest
