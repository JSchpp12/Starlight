#include "starlight/factory/BufferFactory.hpp"

#include "logging/LoggingFactory.hpp"
#include "starlight/core/Exceptions.hpp"

#include <cassert>
#include <memory>
#include <sstream>
#include <string_view>

namespace star::StarBuffers
{
void BufferFactory::CreateAllocation(core::device::StarDevice &device, VmaAllocator &allocator,
                                     const VmaAllocationCreateInfo &allocationCreateInfo,
                                     const vk::BufferCreateInfo &bufferCreateInfo, VmaAllocation &allocation,
                                     vk::Buffer &buffer, const std::string &allocationName,
                                     VmaAllocationInfo &allocationInfo)
{
    vk::Result result;
    {
        auto tmpResult = vmaCreateBuffer(allocator, (VkBufferCreateInfo *)&bufferCreateInfo, &allocationCreateInfo,
                                         (VkBuffer *)&buffer, &allocation, &allocationInfo);
        result = static_cast<vk::Result>(tmpResult);
    }

    if (result != vk::Result::eSuccess)
    {
        LogBufferCreateFailure(result);

        vk::detail::resultCheck(result, "Failed to create buffer via vmaCreateBuffer");
    }

    // Label the VMA allocation in every build configuration so buffers remain identifiable in VMA statistics dumps
    // and memory profilers. This mirrors the naming that used to happen during buffer creation.
    vmaSetAllocationName(allocator, allocation, (allocationName + "_BUFFER").c_str());

#ifndef NDEBUG
    device.assignDebugName(vk::ObjectType::eBuffer, (uint64_t)(VkBuffer)(buffer), allocationName);
#endif
}

void BufferFactory::VerifyBufferCreateInfo(const vk::BufferCreateInfo &createInfo)
{
    assert(createInfo.size != 0 && "size cannot be 0");
    assert(createInfo.sharingMode == vk::SharingMode::eExclusive ||
           (createInfo.sharingMode == vk::SharingMode::eConcurrent && createInfo.queueFamilyIndexCount > 1) &&
               "Must have indices defined for shared resource");
}

void BufferFactory::LogBufferCreateFailure(const vk::Result &result)
{
    std::ostringstream oss;
    oss << "Failed to create buffer with Vulkan error: ";

    if (result == vk::Result::eErrorOutOfDeviceMemory)
    {
        oss << "eErrorOutOfDeviceMemory";
    }
    else
    {
        oss << "Undefined error";
    }

    core::logging::log(boost::log::trivial::error, oss.str());
}

std::shared_ptr<Resources> BufferFactory::CreateResource(core::device::StarDevice &device, VmaAllocator &allocator,
                                                         const VmaAllocationCreateInfo &allocationCreateInfo,
                                                         const vk::BufferCreateInfo &bufferCreateInfo,
                                                         const std::string &allocationName, vk::DeviceSize &offset)
{
    VerifyBufferCreateInfo(bufferCreateInfo);

    vk::Buffer buffer{VK_NULL_HANDLE};
    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;

    CreateAllocation(device, allocator, allocationCreateInfo, bufferCreateInfo, allocation, buffer, allocationName,
                     allocationInfo);

    offset = allocationInfo.offset;

    return std::make_shared<Resources>(allocator, allocation, buffer);
}

void BufferFactory::VerifyNewAllocation(const Buffer::Creators &creators, uint32_t instanceCount,
                                        const vk::DeviceSize &instanceSize)
{
    if (creators.createInfo.size == 0)
    {
        STAR_THROW("Buffer create info must define a non-zero size");
    }

    if (instanceCount == 0)
    {
        STAR_THROW("Instance count must be greater than 0");
    }

    if (instanceSize == 0)
    {
        STAR_THROW("Instance size must be greater than 0");
    }
}

Buffer BufferFactory::FinishNewAllocation(const Buffer::Creators &creators, uint32_t instanceCount,
                                          const vk::DeviceSize &instanceSize, const vk::DeviceSize &minOffsetAlignment,
                                          const vk::DeviceSize &offset, std::shared_ptr<Resources> resources)
{
    const vk::DeviceSize alignmentSize = GetAlignment(instanceSize, minOffsetAlignment);

    return Buffer(std::move(resources), instanceCount, instanceSize, alignmentSize, offset, creators.createInfo.size,
                  creators.createInfo.usage);
}

Buffer BufferFactory::CreateBuffer(core::device::StarDevice &device, const Buffer::Creators &creators,
                                   uint32_t instanceCount, const vk::DeviceSize &instanceSize,
                                   const vk::DeviceSize &minOffsetAlignment)
{
    VerifyNewAllocation(creators, instanceCount, instanceSize);

    auto &allocator = device.getAllocator().get();

    vk::DeviceSize offset = 0;
    auto resources = CreateResource(device, allocator, creators.allocationCreateInfo, creators.createInfo,
                                    creators.allocationName, offset);

    return FinishNewAllocation(creators, instanceCount, instanceSize, minOffsetAlignment, offset, std::move(resources));
}

vk::DeviceSize BufferFactory::GetAlignment(const vk::DeviceSize &instanceSize, const vk::DeviceSize &minOffsetAlignment)
{
    if (minOffsetAlignment > 0)
    {
        return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
    }
    return instanceSize;
}
} // namespace star::StarBuffers
