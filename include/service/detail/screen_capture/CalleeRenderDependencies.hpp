#pragma once

#include "StarTextures/Texture.hpp"
#include "StarBuffers/Buffer.hpp"
#include <starlight/common/Handle.hpp>

#include <vector>

namespace star::service::detail::screen_capture
{
struct CalleeRenderDependencies
{
    Handle commandBufferContainingTarget;
    std::vector<StarBuffers::Buffer> hostVisibleBuffers;
    std::vector<StarTextures::Texture> transferDstTextures;
    std::vector<Handle> targetTextureReadySemaphores;
};

} // namespace star::service::detail::screen_capture