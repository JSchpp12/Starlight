#include "core/command_buffer/ScreenCapture.hpp"

namespace star::core::command_buffer
{
void ScreenCapture::prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
{
}

star::Handle ScreenCapture::registerCommandBuffer(core::device::DeviceContext &context,
                                                  const uint8_t &numFramesInFlight)
{
    return Handle();
}

std::vector<StarTextures::Texture> ScreenCapture::createTransferDstTextures(core::device::DeviceContext &context,
                                                                            const uint8_t &numFramesInFlight) const
{
    auto textures = std::vector<StarTextures::Texture>();

    for (uint8_t i = 0; i < numFramesInFlight; i++){
        // textures[i] = StarTextures::Texture::Builder(context.getDevice(), context.getDevice().getALlocator().get())
        //     .buildUnique();
    }
    
    return textures;
}
} // namespace star::core::command_buffer