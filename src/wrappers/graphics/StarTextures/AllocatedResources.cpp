#include "StarTextures/AllocatedResources.hpp"

star::StarTextures::AllocatedResources::AllocatedResources(vk::Device &device, const vk::Image &image, VmaAllocator &allocator, const VmaAllocation &allocationMemory) : Resources(device, image), allocator(allocator), allocationMemory(allocationMemory)
{
}

star::StarTextures::AllocatedResources::AllocatedResources(vk::Device &device, const vk::Image &image, const std::unordered_map<vk::Format, vk::ImageView> &views, VmaAllocator &allocator, const VmaAllocation &allocationMemory) : Resources(device, image, views), allocator(allocator), allocationMemory(allocationMemory)
{
}

star::StarTextures::AllocatedResources::AllocatedResources(vk::Device &device, const vk::Image &image, const std::unordered_map<vk::Format, vk::ImageView> &views, const vk::Sampler &sampler, VmaAllocator &allocator, const VmaAllocation &allocationMemroy) : Resources(device, image, views, sampler), allocator(allocator), allocationMemory(allocationMemroy)
{
}

star::StarTextures::AllocatedResources::~AllocatedResources()
{
    if (this->sampler.has_value()){
        this->device.destroySampler(this->sampler.value()); 
        this->sampler = std::nullopt;
    }

    for (auto &view : this->views){
        if (view.second){
            this->device.destroyImageView(view.second);
            view.second = VK_NULL_HANDLE;
        }

    }

    vmaDestroyImage(this->allocator, this->image, this->allocationMemory); 
    this->image = VK_NULL_HANDLE;
}
