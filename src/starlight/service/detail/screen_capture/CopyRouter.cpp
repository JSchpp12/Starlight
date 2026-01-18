#include "service/detail/screen_capture/CopyRouter.hpp"

namespace star::service::detail::screen_capture
{

CopyPlan CopyRouter::decide(CalleeRenderDependencies &deps, const Handle &calleeRegistration,
                            const uint8_t &frameInFlightIndex)
{
    common::RoutePath selectedPath;
    vk::Filter selectedFilter;
    decideRoute(deps, selectedPath, selectedFilter);

    return {.resources = decideResourcesToUse(deps, calleeRegistration, frameInFlightIndex),
            .path = selectedPath,
            .blitFilter = std::move(selectedFilter),
            .calleeDependencies = &deps};
}

void CopyRouter::init(DeviceInfo *deviceInfo)
{
    m_deviceCapabilities.init(deviceInfo->device->getPhysicalDevice());
    m_resourceContainer.init(deviceInfo);
}

void CopyRouter::decideRoute(CalleeRenderDependencies &deps, common::RoutePath &route, vk::Filter &copyFilter)
{
    const vk::Format srcFormat = deps.targetTexture.getBaseFormat();

    if (srcFormat == vk::Format::eR8G8B8A8Unorm || srcFormat == vk::Format::eR8G8B8A8Srgb)
    {
        route = common::RoutePath::CopyImageToBufferDirect;
        copyFilter = vk::Filter::eNearest;
        return;
    }

    if (srcFormat == vk::Format::eB8G8R8A8Unorm || srcFormat == vk::Format::eB8G8R8A8Srgb)
    {
        const auto sourceSupport = m_deviceCapabilities.get(srcFormat);

        // need to check if destination image is also supported
        const auto destinationSupport = m_deviceCapabilities.get(vk::Format::eR8G8B8A8Unorm);
        if (sourceSupport.blitSrcOptimal && destinationSupport.blitDstOptimal)
        {
            vk::Filter filter = sourceSupport.linearFilterOK ? vk::Filter::eLinear : vk::Filter::eNearest;

            route = common::RoutePath::BlitImageToRGBAThenCopy;
            copyFilter = filter;
            return;
        }
    }

    route = common::RoutePath::CopyImageToBufferDirect;
    copyFilter = vk::Filter::eNearest;
}

CopyResource CopyRouter::decideResourcesToUse(const CalleeRenderDependencies &deps, const Handle &calleeRegistration,
                                              const uint8_t &frameInFlightIndex)
{
    const vk::Extent2D extent = vk::Extent2D()
                                    .setHeight(deps.targetTexture.getBaseExtent().height)
                                    .setWidth(deps.targetTexture.getBaseExtent().width);
    return m_resourceContainer.giveMeResource(extent, calleeRegistration, frameInFlightIndex);
}
} // namespace star::service::detail::screen_capture