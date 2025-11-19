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
    Handle targetTextureReadySemaphore;
    StarTextures::Texture targetTexture;
    std::vector<StarBuffers::Buffer> hostVisibleBuffers;
};

} // namespace star::service::detail::screen_capture