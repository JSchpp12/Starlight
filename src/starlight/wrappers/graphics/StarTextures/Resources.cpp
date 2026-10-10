#include "StarTextures/Resources.hpp"

star::StarTextures::Resources::Resources(const vk::Image &image) : image(image)
{
}

star::StarTextures::Resources::Resources(const vk::Image &image,
                                         const TextureView &views)
    : image(image), views(views)
{
}

star::StarTextures::Resources::Resources(const vk::Image &image,
                                         const TextureView &views,
                                         const vk::Sampler &sampler)
    : image(image), views(views), sampler(std::make_optional<vk::Sampler>(sampler))
{
}

void star::StarTextures::Resources::cleanupRender(vk::Device &device){
    if (this->sampler.has_value()){
        device.destroySampler(sampler.value());
    }
    sampler = std::nullopt; 

    views.destroy(device);
}