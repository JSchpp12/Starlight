#include "StarTextures/Resources.hpp"

star::StarTextures::Resources::Resources(vk::Device &device, const vk::Image &image) : device(device), image(image)
{
}

star::StarTextures::Resources::Resources(vk::Device &device, const vk::Image &image, const std::unordered_map<vk::Format, vk::ImageView> &views) : device(device), image(image), views(views)
{
}

star::StarTextures::Resources::Resources(vk::Device &device, const vk::Image &image, const std::unordered_map<vk::Format, vk::ImageView> &views, const vk::Sampler &sampler) : device(device), image(image), views(views), sampler(std::make_optional<vk::Sampler>(sampler))
{
}

star::StarTextures::Resources::~Resources()
{
    if (this->sampler.has_value())
        this->device.destroySampler(this->sampler.value()); 

    for (auto &view : this->views){
        if (view.second)
            this->device.destroyImageView(view.second);
    }
}