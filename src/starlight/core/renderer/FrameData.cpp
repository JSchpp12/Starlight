#include "core/renderer/FrameData.hpp"

#include "core/device/DeviceContext.hpp"
#include "starlight/wrappers/graphics/StarSemaphore.hpp"

#include <star_common/TypeRegistry.hpp>

#include <cassert>

namespace star::core::renderer
{
namespace
{
/// All frame-resource roles share a single handle type ("stFrameRole"); the role
/// itself is the handle's id, minted from its name. One type, many ids -- adding
/// a role (particles, ...) is a new name -> new id, never a new type.
uint16_t frameRoleType()
{
    static const uint16_t type = common::HandleTypeRegistry::instance().registerType("stFrameRole");
    return type;
}

common::TypeRegistry &frameRoleIdRegistry()
{
    static common::TypeRegistry registry;
    return registry;
}
} // namespace

Handle roleHandle(std::string_view roleName)
{
    return Handle{.type = frameRoleType(), .id = frameRoleIdRegistry().registerType(roleName)};
}

FrameData &FrameData::add(std::shared_ptr<ManagerController::RenderResource::Buffer> controller)
{
    m_resources.emplace_back(Handle{}, Resource{DrivenBuffer{std::move(controller)}});
    return *this;
}

FrameData &FrameData::add(std::shared_ptr<ManagerController::RenderResource::Buffer> controller, Handle role)
{
    m_roleIndex[role] = m_resources.size();
    m_resources.emplace_back(role, Resource{DrivenBuffer{std::move(controller)}});
    return *this;
}

FrameData &FrameData::add(BorrowedBuffer borrowed, Handle role)
{
    m_roleIndex[role] = m_resources.size();
    m_resources.emplace_back(role, Resource{std::move(borrowed)});
    return *this;
}

FrameData &FrameData::add(TextureHandle texture, Handle role)
{
    m_roleIndex[role] = m_resources.size();
    m_resources.emplace_back(role, Resource{std::move(texture)});
    return *this;
}

FrameData &FrameData::add(FixedBufferHandle handle, Handle role)
{
    m_roleIndex[role] = m_resources.size();
    m_resources.emplace_back(role, Resource{std::move(handle)});
    return *this;
}

FrameData &FrameData::add(OwnedBuffer buffers, Handle role)
{
    m_roleIndex[role] = m_resources.size();
    m_resources.emplace_back(role, Resource{std::move(buffers)});
    return *this;
}

FrameData &FrameData::add(BorrowedTexture textures, Handle role)
{
    m_roleIndex[role] = m_resources.size();
    m_resources.emplace_back(role, Resource{std::move(textures)});
    return *this;
}

FrameData &FrameData::add(OwnedTexture textures, Handle role)
{
    m_roleIndex[role] = m_resources.size();
    m_resources.emplace_back(role, Resource{std::move(textures)});
    return *this;
}

void FrameData::prepRender(core::device::DeviceContext &context, uint8_t numFramesInFlight)
{
    for (auto &entry : m_resources)
        if (auto *d = std::get_if<DrivenBuffer>(&entry.second))
            d->controller->prepRender(context, numFramesInFlight);
    m_pending.reserve(m_resources.size());
}

FrameData::FrameUpdateResult FrameData::frameUpdate(core::device::DeviceContext &context,
                                                    std::optional<core::graphics::SemaphoreInfo> priorSync)
{
    const uint8_t fi = context.frameTracker().getCurrent().getFrameInFlightIndex();
    m_pending.clear();

    for (auto &entry : m_resources)
    {
        // Only driven (owned) buffer controllers are updated here. Every other
        // variant is binding-only -- owned/driven elsewhere, or not at all.
        auto *d = std::get_if<DrivenBuffer>(&entry.second);
        if (!d)
            continue;

        const auto [submitted, semaphore] = d->controller->submitUpdateIfNeeded(context, fi, priorSync);
        if (submitted)
        {
            m_pending.push_back(PendingWait{.handle = d->controller->getHandle(fi),
                                            .signalValue = semaphore->signalValue,
                                            .semaphore = semaphore->vkSemaphore,
                                            .waitStage = d->controller->waitStage()});
        }
    }

    return FrameUpdateResult{std::span<const PendingWait>{m_pending}};
}

const FrameData::Resource &FrameData::resource(Handle role) const
{
    const auto it = m_roleIndex.find(role);
    assert(it != m_roleIndex.end() && "resource: role not registered with this FrameData");
    return m_resources.at(it->second).second;
}

ManagerController::RenderResource::Buffer *FrameData::controller(Handle role) const
{
    const auto &res = resource(role);
    if (const auto *driven = std::get_if<DrivenBuffer>(&res))
        return driven->controller.get();
    if (const auto *borrowed = std::get_if<BorrowedBuffer>(&res))
        return borrowed->controller;
    assert(false && "controller: role is not a buffer slot");
    return nullptr;
}
} // namespace star::core::renderer