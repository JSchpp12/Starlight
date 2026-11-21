#include "core/renderer/RenderingContext.hpp"

#include "core/device/DeviceContext.hpp"
#include "core/device/managers/Image.hpp"

namespace star::core::renderer
{
void RenderingContext::initListener(common::EventBus &eventBus)
{
    if (m_imageResizeListener != nullptr)
        return;

    m_imageResizeListener = std::make_unique<ImageResizeListener>(*this);
    m_imageResizeListener->init(eventBus);
}

void RenderingContext::cleanupListener(common::EventBus &eventBus)
{
    if (m_imageResizeListener == nullptr)
        return;

    m_imageResizeListener->cleanup(eventBus);
    m_imageResizeListener.reset();
}

void RenderingContext::onManagerReservedMemoryUpdate(const star::event::ManagerReservedMemoryUpdate &event,
                                                     bool &keepAlive)
{
    // Stay subscribed for every manager reallocation: leaving keepAlive false unsubscribes this listener, so another
    // manager's reallocation would drop it before the Image manager ever grows.
    keepAlive = true;

    const auto imageType = common::HandleTypeRegistry::instance().getType(common::special_types::GetImageTypeName);
    if (!imageType.has_value() || event.getManagerHandleType() != *imageType)
        return;

    auto *imageManager = static_cast<core::device::manager::Image *>(event.getManager());
    if (imageManager == nullptr)
        return;

    // Every cached pointer points into the manager's record storage, which just moved.
    for (auto &[handle, cached] : recordDependentImage.getRecords())
    {
        auto *record = imageManager->get(handle);
        if (record == nullptr || record->getTexture() == nullptr)
            continue;

        cached = record->getTexture();
    }
}

void RenderingContext::addBufferToRenderingContext(star::core::device::DeviceContext& context, const Handle& handle)
{
    assert(handle.getType() ==
           common::HandleTypeRegistry::instance().getType(common::special_types::BufferTypeName).value());

    bufferTransferRecords.manualInsert(
        handle, context.getManagerRenderResource().getBuffer(context.getDeviceID(), handle).getVulkanBuffer());
}
} // namespace star::core::renderer
