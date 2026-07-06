#pragma once

#include "Allocator.hpp"
#include "StarBuffers/Buffer.hpp"
#include "StarCommandBuffer.hpp"
#include "StarCommandPool.hpp"
#include "StarQueueFamily.hpp"
#include "StarTextures/Texture.hpp"
#include "TransferRequest_Buffer.hpp"
#include "TransferRequest_Texture.hpp"
#include "core/graphics/GPUWorkSyncInfo.hpp"
#include "device/StarDevice.hpp"

#include <boost/atomic.hpp>
#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>
#include <queue>
#include <vector>

namespace star::job
{

class TransferManagerThread
{
  public:
    struct InterThreadRequest
    {
        core::graphics::GPUWorkSyncInfo workSyncInfo;
        boost::atomic<bool> *gpuDoneNotificationToMain = nullptr;
        std::unique_ptr<TransferRequest::Buffer> bufferTransferRequest = nullptr;
        std::unique_ptr<TransferRequest::Texture> textureTransferRequest = nullptr;
        std::optional<std::unique_ptr<StarBuffers::Buffer> *> resultingBuffer = std::nullopt;
        std::optional<std::unique_ptr<StarTextures::Texture> *> resultingTexture = std::nullopt;

        InterThreadRequest() = default;

        InterThreadRequest(boost::atomic<bool> *gpuDoneNotificationToMain,
                           std::unique_ptr<TransferRequest::Buffer> bufferTransferRequest,
                           std::unique_ptr<StarBuffers::Buffer> &resultingBuffer)
            : workSyncInfo(), gpuDoneNotificationToMain(gpuDoneNotificationToMain),
              bufferTransferRequest(std::move(bufferTransferRequest)), resultingBuffer(&resultingBuffer)
        {
        }

        InterThreadRequest(boost::atomic<bool> *gpuDoneNotificationToMain,
                           std::unique_ptr<TransferRequest::Buffer> bufferTransferRequest,
                           std::unique_ptr<StarBuffers::Buffer> &resultingBuffer,
                           core::graphics::GPUWorkSyncInfo workSyncInfo)
            : workSyncInfo(std::move(workSyncInfo)), gpuDoneNotificationToMain(gpuDoneNotificationToMain),
              bufferTransferRequest(std::move(bufferTransferRequest)), resultingBuffer(&resultingBuffer)
        {
        }

        InterThreadRequest(boost::atomic<bool> *gpuDoneNotificationToMain,
                           std::unique_ptr<TransferRequest::Texture> textureTransferRequest,
                           std::unique_ptr<StarTextures::Texture> &resultingTexture,
                           core::graphics::GPUWorkSyncInfo workSyncInfo)
            : workSyncInfo(std::move(workSyncInfo)), gpuDoneNotificationToMain(gpuDoneNotificationToMain),
              textureTransferRequest(std::move(textureTransferRequest)), resultingTexture(&resultingTexture)
        {
        }
        InterThreadRequest(boost::atomic<bool> *gpuDoneNotificationToMain,
                           std::unique_ptr<TransferRequest::Texture> textureTransferRequest,
                           std::unique_ptr<StarTextures::Texture> &resultingTexture)
            : workSyncInfo(), gpuDoneNotificationToMain(gpuDoneNotificationToMain),
              textureTransferRequest(std::move(textureTransferRequest)), resultingTexture(&resultingTexture)
        {
        }

        void reset()
        {
            gpuDoneNotificationToMain = nullptr;
            bufferTransferRequest = nullptr;
            textureTransferRequest = nullptr;
            resultingBuffer = std::nullopt;
            resultingTexture = std::nullopt;
        }
    };

    class ProcessRequestInfo
    {
      public:
        std::shared_ptr<StarCommandPool> commandPool = nullptr;
        std::unique_ptr<StarCommandBuffer> commandBuffer = nullptr;

        ProcessRequestInfo(std::shared_ptr<StarCommandPool> commandPool,
                           std::unique_ptr<StarCommandBuffer> commandBuffer)
            : commandPool(std::move(commandPool)), commandBuffer(std::move(commandBuffer)) {};

        void setInProcessDeps(std::unique_ptr<StarBuffers::Buffer> nInProcessTransferSrcBuffer)
        {
            this->inProcessTransferSrcBuffer = std::move(nInProcessTransferSrcBuffer);
        }

        void markAsAvailable(vk::Device device)
        {
            if (inProcessTransferSrcBuffer)
            {
                inProcessTransferSrcBuffer->cleanupRender(device);
            }

            this->inProcessTransferSrcBuffer.reset();
            this->inProcessTransferSrcBuffer = nullptr;
        }

        bool isMarkedAsAvailable() const
        {
            return this->inProcessTransferSrcBuffer == nullptr;
        }

      private:
        std::unique_ptr<StarBuffers::Buffer> inProcessTransferSrcBuffer = nullptr;
    };

    static void CreateBuffer(vk::Device device, VmaAllocator allocator, StarQueue &queue,
                             const vk::PhysicalDeviceProperties &deviceProperties,
                             const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
                             ProcessRequestInfo &processInfo, TransferRequest::Buffer *newBufferRequest,
                             std::unique_ptr<StarBuffers::Buffer> *resultingBuffer,
                             boost::atomic<bool> *gpuDoneSignalMain, core::graphics::GPUWorkSyncInfo &syncInfo);

    static void CreateTexture(vk::Device device, VmaAllocator allocator, StarQueue &queue,
                              const vk::PhysicalDeviceProperties &deviceProperties,
                              const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
                              ProcessRequestInfo &processInfo, TransferRequest::Texture *newTextureRequest,
                              std::unique_ptr<StarTextures::Texture> *resultingTexture,
                              boost::atomic<bool> *gpuDoneSignalToMain, core::graphics::GPUWorkSyncInfo &syncInfo);

    static void CheckForCleanups(vk::Device device, std::queue<std::unique_ptr<ProcessRequestInfo>> &processingInfos);
};

} // namespace star::job
