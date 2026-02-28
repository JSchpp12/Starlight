#pragma once

#include "StarTextures/Resources.hpp"
#include "core/device/StarDevice.hpp"

namespace star::StarTextures
{
class AllocatedResources : public Resources
{
  public:
    VmaAllocation allocationMemory = VmaAllocation();

    AllocatedResources(const vk::Image &image, const VmaAllocation &allocationMemory, VmaAllocator allocator)
        : Resources(image), allocationMemory(allocationMemory), m_allocator(std::move(allocator)){};
    AllocatedResources(const vk::Image &image, const std::unordered_map<vk::Format, vk::ImageView> &views,
                       const VmaAllocation &allocationMemory, VmaAllocator allocator)
        : Resources(image, views), allocationMemory(allocationMemory), m_allocator(std::move(allocator))
    {
    }
    AllocatedResources(const vk::Image &image, const std::unordered_map<vk::Format, vk::ImageView> &views,
                       const vk::Sampler &sampler, const VmaAllocation &allocationMemory, VmaAllocator allocator)
        : Resources(image, views, sampler), allocationMemory(allocationMemory), m_allocator(allocator)
    {
    }

    virtual ~AllocatedResources() = default;

    virtual void cleanupRender(vk::Device &device) override
    {
        if (this->image != VK_NULL_HANDLE)
        {
            Resources::cleanupRender(device);
            vmaDestroyImage(m_allocator, this->image, this->allocationMemory);

            this->image = VK_NULL_HANDLE;
        }
    }

  private:
    VmaAllocator m_allocator;
};
} // namespace star::StarTextures