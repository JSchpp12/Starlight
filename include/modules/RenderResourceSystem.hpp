#pragma once

#include "BufferHandle.hpp"
#include "DeviceContext.hpp"
#include "StarBuffers/Buffer.hpp"
#include "StarCommandBuffer.hpp"
#include "Vertex.hpp"

#include <vulkan/vulkan.hpp>

#include <functional>
#include <memory>
#include <stack>
#include <vector>

namespace star
{
class RenderResourceSystem
{
  public:
    static void registerCallbacks(
        std::function<void(core::device::DeviceContext &, const int &, const vk::Extent2D &)> initCallback,
        std::function<void(core::device::DeviceContext &)> destroyCallback);

    static void registerLoadGeomDataCallback(
        std::function<std::pair<std::unique_ptr<StarBuffers::Buffer>, std::unique_ptr<StarBuffers::Buffer>>(
            core::device::DeviceContext &, BufferHandle, BufferHandle)>);

    static void registerSetDrawInfoCallback(
        std::function<void(const uint32_t &, const uint32_t &, const uint32_t &)> setGeometryDataOffsetCallback);

    static void bind(const Handle &resource, vk::CommandBuffer &commandBuffer);

    static void bind(const BufferHandle &buffer, vk::CommandBuffer &commandBuffer);

    static void init(core::device::DeviceContext &device, const int &numFramesInFlight, const vk::Extent2D &screensize);

    static void cleanup(core::device::DeviceContext &device);

    friend class StarEngine;

  private:
    static std::stack<std::function<void(core::device::DeviceContext &, const int &, const vk::Extent2D &)>>
        initCallbacks;
    static std::stack<std::function<void(core::device::DeviceContext &)>> destroyCallbacks;
    static std::stack<
        std::function<std::pair<std::unique_ptr<StarBuffers::Buffer>, std::unique_ptr<StarBuffers::Buffer>>(
            core::device::DeviceContext &, BufferHandle, BufferHandle)>>
        loadGeometryCallbacks;
    static std::stack<std::function<void(const uint32_t &, const uint32_t &, const uint32_t &)>>
        geometryDataOffsetCallbacks;
    static std::vector<std::unique_ptr<StarBuffers::Buffer>> buffers;

    static void preparePrimaryGeometry(core::device::DeviceContext &device);

    static void runInits(core::device::DeviceContext &device, const int &numFramesInFlight,
                         const vk::Extent2D &screensize);

    static void runDestroys(core::device::DeviceContext &device);

    static void bindBuffer(const uint32_t &bufferId, vk::CommandBuffer &commandBuffer, const size_t &offset = 0);
};
} // namespace star