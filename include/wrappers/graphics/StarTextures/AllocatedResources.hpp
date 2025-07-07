#pragma once

#include "StarTextures/Resources.hpp"

namespace star::StarTextures
{
class AllocatedResources : public Resources
{
  private:
    VmaAllocator &allocator;

  public:
    VmaAllocation allocationMemory = VmaAllocation();

    AllocatedResources(vk::Device &device, const vk::Image &image, VmaAllocator &allocator,
                       const VmaAllocation &allocationMemory);
    AllocatedResources(vk::Device &device, const vk::Image &image,
                       const std::unordered_map<vk::Format, vk::ImageView> &views, VmaAllocator &allocator,
                       const VmaAllocation &allocationMemory);
    AllocatedResources(vk::Device &device, const vk::Image &image,
                       const std::unordered_map<vk::Format, vk::ImageView> &views, const vk::Sampler &sampler,
                       VmaAllocator &allocator, const VmaAllocation &allocationMemroy);

    ~AllocatedResources() override;
};
} // namespace star::StarTextures