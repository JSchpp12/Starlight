#pragma once

#include "Common.hpp"
#include "ManagedHandleContainer.hpp"
#include "StarBuffers/Buffer.hpp"
#include "StarTextures/Texture.hpp"
#include "data_structure/dynamic/ThreadSharedObjectPool.hpp"
#include "wrappers/graphics/policies/GenericBufferCreateAllocatePolicy.hpp"

#include <array>
#include <string_view>
#include <vector>

namespace star::service::detail::screen_capture
{

class CopyResourcesContainer
{
  public:
    struct ImageChunk
    {
        std::vector<star::StarTextures::Texture> textures;

        void cleanupRender(core::device::StarDevice &device)
        {
            for (auto &tex : textures)
            {
                tex.cleanupRender(device.getVulkanDevice());
            }
        }
    };

    CopyResourcesContainer(wrappers::graphics::policies::GenericBufferCreateAllocatePolicy createPolicy)
        : m_blitTexturePool(common::ScreenCaptureServiceCalleeTypeName),
          m_hostVisibleBufferPool(std::move(createPolicy))
    {
    }

    data_structure::dynamic::ThreadSharedObjectPool<
        star::StarBuffers::Buffer, wrappers::graphics::policies::GenericBufferCreateAllocatePolicy, 1> &
    getBufferPool()
    {
        return m_hostVisibleBufferPool;
    }

    core::ManagedHandleContainer<ImageChunk, 10> &getBlitTexturePool()
    {
        return m_blitTexturePool;
    }

    void cleanupRender(core::device::StarDevice &device)
    {
        m_blitTexturePool.cleanupAll(&device);
    }

  private:
    core::ManagedHandleContainer<ImageChunk, 10> m_blitTexturePool;

    data_structure::dynamic::ThreadSharedObjectPool<star::StarBuffers::Buffer,
                                                    wrappers::graphics::policies::GenericBufferCreateAllocatePolicy, 1>
        m_hostVisibleBufferPool;
};
} // namespace star::service::detail::screen_capture