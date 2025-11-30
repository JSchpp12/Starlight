#pragma once

#include "StarTextures/Texture.hpp"

#include <starlight/common/Handle.hpp>

#include <optional>

namespace star::service::detail::screen_capture
{
struct CalleeRenderDependencies
{
    class Builder
    {
      public:
        Builder &setCommandBufferContainingTarget(Handle commandBufferHandle)
        {
            m_commandBufferContainingTarget = std::move(commandBufferHandle);
            return *this;
        }
        Builder &setTargetTexture(StarTextures::Texture targetTexture)
        {
            m_targetTexture = std::move(targetTexture);
            return *this;
        }
        Builder &setTargetTextureReadySemaphore(Handle targetTextureReadySemaphore)
        {
            m_targetTextureReadySemaphore = std::move(targetTextureReadySemaphore);
            return *this;
        }
        CalleeRenderDependencies build()
        {
            return {.commandBufferContainingTarget = std::move(m_commandBufferContainingTarget),
                    .targetTexture = std::move(m_targetTexture),
                    .targetTextureReadySemaphore = std::move(m_targetTextureReadySemaphore)};
        }

      private:
        Handle m_commandBufferContainingTarget;
        StarTextures::Texture m_targetTexture;
        std::optional<Handle> m_targetTextureReadySemaphore;
    };

    void cleanupRender()
    {
    }
    Handle commandBufferContainingTarget;
    StarTextures::Texture targetTexture;
    std::optional<Handle> targetTextureReadySemaphore = std::nullopt;
};

} // namespace star::service::detail::screen_capture