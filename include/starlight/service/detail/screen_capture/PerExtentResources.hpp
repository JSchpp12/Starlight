#pragma once

#include "CopyResourcesContainer.hpp"
#include "DeviceInfo.hpp"
#include "MappedHandleContainer.hpp"
#include "StarBuffers/Buffer.hpp"
#include "StarTextures/Texture.hpp"

#include <absl/container/flat_hash_map.h>
#include <vulkan/vulkan.hpp>

namespace star::service::detail::screen_capture
{
struct Extent2DHash
{
    std::size_t operator()(const vk::Extent2D &e) const noexcept;
};
struct Extent2DEqual
{
    bool operator()(const vk::Extent2D &a, const vk::Extent2D &b) const noexcept;
};

struct CopyResource
{
    struct ThreadSharedBufferInfo
    {
        Handle containerRegistration;
        StarBuffers::Buffer &hostVisibleBuffer;
        CopyResourcesContainer *container = nullptr;
    };

    ThreadSharedBufferInfo bufferInfo;
    std::optional<vk::Image> blitTargetTexture = std::nullopt;
};

/// Container relating extent to a container of resources which can be used for copy operations
class PerExtentResources
{
  public:
    PerExtentResources() = default;
    explicit PerExtentResources(DeviceInfo *deviceInfo) : m_resources(), m_deviceInfo(deviceInfo)
    {
    }

    void init(DeviceInfo *deviceInfo)
    {
        m_deviceInfo = deviceInfo;
    }

    CopyResource giveMeResource(const vk::Extent2D &targetExtent, const Handle &calleeRegistration, const uint8_t &frameInFlightIndex);

    void cleanupRender();

  private:
    absl::flat_hash_map<vk::Extent2D, std::unique_ptr<CopyResourcesContainer>, Extent2DHash, Extent2DEqual> m_resources;
    DeviceInfo *m_deviceInfo = nullptr;
};
} // namespace star::service::detail::screen_capture