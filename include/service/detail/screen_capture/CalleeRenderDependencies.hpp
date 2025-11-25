#pragma once

#include "StarBuffers/Buffer.hpp"
#include "StarTextures/Texture.hpp"

#include <boost/atomic/atomic.hpp>
#include <starlight/common/Handle.hpp>

#include <vector>

namespace star::service::detail::screen_capture
{
struct CompleteCalleeRenderDependencies
{
    StarBuffers::Buffer *buffer; 
    boost::atomic<bool> *secondaryThreadWriteIsDone;
    Handle *commandBufferContainingTarget;
    Handle *targetTextureReadySemaphore;
    StarTextures::Texture *targetTexture;
};

struct CalleeRenderDependencies
{
    struct BufferInfo
    {
        StarBuffers::Buffer bufferObject;
        std::unique_ptr<boost::atomic<bool>> secondaryThreadWriteIsDone;
    };

    class Builder
    {
      public:
        Builder &addBufferInfo(StarBuffers::Buffer buffer, bool initalFlagValue)
        {
            m_bufferInfo.push_back(
                {.bufferObject = std::move(buffer),
                 .secondaryThreadWriteIsDone = std::make_unique<boost::atomic<bool>>(initalFlagValue)});
            return *this;
        }
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
            return {.bufferInfo = std::move(m_bufferInfo),
                    .commandBufferContainingTarget = std::move(m_commandBufferContainingTarget),
                    .targetTextureReadySemaphore = std::move(m_targetTextureReadySemaphore),
                    .targetTexture = std::move(m_targetTexture)};
        }

      private:
        std::vector<BufferInfo> m_bufferInfo;
        Handle m_commandBufferContainingTarget;
        Handle m_targetTextureReadySemaphore;
        StarTextures::Texture m_targetTexture;
    };
    std::vector<BufferInfo> bufferInfo;
    Handle commandBufferContainingTarget;
    Handle targetTextureReadySemaphore;
    StarTextures::Texture targetTexture;
};

} // namespace star::service::detail::screen_capture