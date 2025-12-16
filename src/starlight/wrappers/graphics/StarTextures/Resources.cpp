#include "StarTextures/Resources.hpp"

star::StarTextures::Resources::Resources(const vk::Image &image) : image(image)
{
}

star::StarTextures::Resources::Resources(const vk::Image &image,
                                         const std::unordered_map<vk::Format, vk::ImageView> &views)
    : image(image), views(views)
{
}

star::StarTextures::Resources::Resources(const vk::Image &image,
                                         const std::unordered_map<vk::Format, vk::ImageView> &views,
                                         const vk::Sampler &sampler)
    : image(image), views(views), sampler(std::make_optional<vk::Sampler>(sampler))
{
}

void star::StarTextures::Resources::cleanupRender(vk::Device &device){
    if (this->sampler.has_value()){
        device.destroySampler(sampler.value());
    }
    sampler = std::nullopt; 

    for (auto &view : views){
        if (view.second){
            device.destroyImageView(view.second);
        }
    }

    views.clear();
}