#pragma once

#include "StarTextures/Texture.hpp"
#include "StarBuffers/Buffer.hpp"
#include "Handle.hpp"

#include <vector>

namespace star::service::detail::screen_capture
{
struct CalleeRenderDependencies
{
    std::vector<StarTextures::Texture> m_transferDstTextures;
    std::vector<StarBuffers::Buffer> m_hostVisibleBuffers;
    std::vector<Handle> m_targetTexturesReadySemaphores;
};

} // namespace star::service::detail::screen_capture