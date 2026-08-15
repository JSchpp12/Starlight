#include "core/renderer/DescriptorRecipe.hpp"

#include "core/device/managers/DescriptorPool.hpp"
#include "starlight/core/waiter/one_shot/CreateDescriptorsOnEventPolicy.hpp"

#include <star_common/EventBus.hpp>
#include <star_common/HandleTypeRegistry.hpp>

#include <algorithm>
#include <cassert>
#include <map>
#include <variant>

namespace star::core::renderer
{
namespace
{
/// All shader-info slots share a single handle type ("stShaderInfo"); the slot
/// is the handle's id, minted from its name. One type, many ids -- adding a
/// shader-info (Static, Dynamic, Global, ...) is a new name -> new id, never a
/// new type. Mirrors how roles are minted in FrameData.
uint16_t shaderInfoType()
{
    static const uint16_t type = common::HandleTypeRegistry::instance().registerType("stShaderInfo");
    return type;
}

common::TypeRegistry &shaderInfoIdRegistry()
{
    static common::TypeRegistry registry;
    return registry;
}

/// Order shader-info handles by (type, id) so grouping is deterministic even
/// across distinct handle types.
struct HandleLess
{
    bool operator()(const Handle &lhs, const Handle &rhs) const
    {
        if (lhs.type != rhs.type)
            return lhs.type < rhs.type;
        return lhs.id < rhs.id;
    }
};
} // namespace

Handle shaderInfoHandle(std::string_view name)
{
    return Handle{.type = shaderInfoType(), .id = shaderInfoIdRegistry().registerType(name)};
}

DescriptorRecipe::DescriptorRecipe(core::device::DeviceContext *context,
                                   std::vector<std::pair<Handle, std::unique_ptr<StarShaderInfo> *>> shaderInfoOuts,
                                   std::vector<Binding> bindings, std::vector<StarRenderGroup> *renderGroups,
                                   Handle groupShaderInfo, RenderingTargetInfo renderingTargetInfo,
                                   Handle commandBuffer, std::function<void(core::device::DeviceContext &)> onReady)
    : m_context(context), m_shaderInfoOuts(std::move(shaderInfoOuts)), m_bindings(std::move(bindings)),
      m_renderGroups(renderGroups), m_groupShaderInfo(groupShaderInfo),
      m_renderingTargetInfo(std::move(renderingTargetInfo)), m_commandBuffer(commandBuffer),
      m_onReady(std::move(onReady))
{
}

int DescriptorRecipe::operator()()
{
    assert(m_context && !m_bindings.empty() && !m_shaderInfoOuts.empty());

    const uint8_t numFramesInFlight = m_context->frameTracker().getSetup().getNumFramesInFlight();

    StarDescriptorPool *defaultPool{nullptr};
    {
        const Handle dHandle{.type = common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                                 core::device::manager::GetDescriptorPoolTypeName),
                             .id = 0};
        defaultPool = m_context->getDescriptorPoolManager().get(dHandle)->pool.get();
    }
    assert(defaultPool != nullptr);

    // Group bindings by shaderInfo, then by set. The layout is data: built from
    // the binding table. Each StarShaderInfo owns its layouts
    std::map<Handle, std::map<uint32_t, std::vector<const Binding *>>, HandleLess> byShaderInfo;
    for (const auto &b : m_bindings)
        byShaderInfo[b.shaderInfo][b.set].push_back(&b);

    // Build one StarShaderInfo per registered sink, in registration order.
    std::vector<std::shared_ptr<StarDescriptorSetLayout>> groupLayouts;
    for (auto &[shaderInfo, out] : m_shaderInfoOuts)
    {
        const auto it = byShaderInfo.find(shaderInfo);
        assert(it != byShaderInfo.end() && "shaderInfo registered but has no bindings");
        const auto &bySet = it->second;

        auto builder =
            StarShaderInfo::Builder(m_context->getDeviceID(), m_context->getDevice(), *defaultPool, numFramesInFlight);

        // One layout per set, in ascending set order.
        std::vector<uint32_t> setOrder;
        for (const auto &[set, bindings] : bySet)
        {
            setOrder.push_back(set);
            auto sorted = bindings;
            std::sort(sorted.begin(), sorted.end(),
                      [](const Binding *a, const Binding *b) { return a->binding < b->binding; });
            StarDescriptorSetLayout::Builder layoutBuilder;
            for (const auto *b : sorted)
                layoutBuilder.addBinding(b->binding, b->type, b->stage);
            builder.addSetLayout(layoutBuilder.build(m_context->getDevice()));
        }

        // Per frame-in-flight, per set, resolve each role's resource.
        for (uint8_t i = 0; i < numFramesInFlight; i++)
        {
            builder.startOnFrameIndex(i);
            for (const auto &set : setOrder)
            {
                builder.startSet();
                auto sorted = bySet.at(set);
                std::sort(sorted.begin(), sorted.end(),
                          [](const Binding *a, const Binding *b) { return a->binding < b->binding; });
                for (const auto *b : sorted)
                {
                    const auto &res = b->source->resource(b->role);
                    if (std::holds_alternative<FrameData::DrivenBuffer>(res))
                    {
                        const auto &d = std::get<FrameData::DrivenBuffer>(res);
                        builder.add(StarShaderInfo::BufferInfo{d.controller->getHandle(i)});
                    }
                    else if (std::holds_alternative<FrameData::BorrowedBuffer>(res))
                    {
                        const auto &bb = std::get<FrameData::BorrowedBuffer>(res);
                        builder.add(StarShaderInfo::BufferInfo{bb.controller->getHandle(i)});
                    }
                    else if (std::holds_alternative<FrameData::FixedBufferHandle>(res))
                    {
                        const auto &h = std::get<FrameData::FixedBufferHandle>(res);
                        builder.add(StarShaderInfo::BufferInfo{h.handle});
                    }
                    else if (std::holds_alternative<FrameData::OwnedBuffer>(res))
                    {
                        const auto &b = std::get<FrameData::OwnedBuffer>(res);
                        builder.add(StarShaderInfo::BufferInfo{b.buffers[i].get()});
                    }
                    else if (std::holds_alternative<FrameData::TextureHandle>(res))
                    {
                        const auto &t = std::get<FrameData::TextureHandle>(res);
                        builder.add(StarShaderInfo::TextureInfo{t.textureHandle, t.layout, t.format});
                    }
                    else if (std::holds_alternative<FrameData::BorrowedTexture>(res))
                    {
                        const auto &t = std::get<FrameData::BorrowedTexture>(res);
                        builder.add(StarShaderInfo::TextureInfo{t.textures[i], t.layout, t.format});
                    }
                    else if (std::holds_alternative<FrameData::OwnedTexture>(res))
                    {
                        const auto &t = std::get<FrameData::OwnedTexture>(res);
                        builder.add(StarShaderInfo::TextureInfo{t.textures[i].get(), t.layout, t.format});
                    }
                }
            }
        }

        if (shaderInfo == m_groupShaderInfo)
            groupLayouts = builder.getCurrentSetLayouts();

        *out = builder.build();
    }

    if (m_renderGroups)
    {
        assert(m_commandBuffer.isInitialized() && "render-group notification requires a command buffer");
        auto groupBuilder =
            StarShaderInfo::Builder(m_context->getDeviceID(), m_context->getDevice(), *defaultPool, numFramesInFlight);
        for (const auto &layout : groupLayouts)
            groupBuilder.addSetLayout(layout);
        for (auto &group : *m_renderGroups)
            group.onDescriptorPoolReady(*m_context, groupBuilder, m_renderingTargetInfo, m_commandBuffer);
    }

    // Generic post-build hook: runs after every StarShaderInfo is built
    if (m_onReady)
        m_onReady(*m_context);

    return 0;
}

DescriptorRecipe::Builder::Builder(common::EventBus &bus, core::device::DeviceContext &context,
                                   std::string_view eventName)
    : m_bus(bus), m_eventType(common::HandleTypeRegistry::instance().registerType(eventName)), m_context(&context)
{
}

DescriptorRecipe::Builder &DescriptorRecipe::Builder::setShaderInfoOut(Handle shaderInfo,
                                                                       std::unique_ptr<StarShaderInfo> *out)
{
    m_shaderInfoOuts.emplace_back(shaderInfo, out);
    return *this;
}

DescriptorRecipe::Builder &DescriptorRecipe::Builder::addBinding(Handle shaderInfo, uint32_t set,
                                                                 std::shared_ptr<FrameData> source, Handle role,
                                                                 uint32_t binding, vk::DescriptorType type,
                                                                 vk::ShaderStageFlags stage)
{
    m_bindings.push_back(Binding{shaderInfo, set, std::move(source), role, binding, type, stage});
    return *this;
}

DescriptorRecipe::Builder &DescriptorRecipe::Builder::setRenderGroups(Handle groupShaderInfo,
                                                                      std::vector<StarRenderGroup> *groups,
                                                                      RenderingTargetInfo info, Handle commandBuffer)
{
    m_groupShaderInfo = groupShaderInfo;
    m_renderGroups = groups;
    m_renderingTargetInfo = std::move(info);
    m_commandBuffer = commandBuffer;
    return *this;
}

DescriptorRecipe::Builder &DescriptorRecipe::Builder::setOnShaderInfoReady(
    std::function<void(core::device::DeviceContext &)> onReady)
{
    m_onReady = std::move(onReady);
    return *this;
}

void DescriptorRecipe::Builder::build()
{
    assert(m_context && !m_shaderInfoOuts.empty() && !m_bindings.empty());
    if (m_renderGroups)
        assert(m_commandBuffer.isInitialized() && "render-group notification requires a command buffer");

    DescriptorRecipe recipe(m_context, std::move(m_shaderInfoOuts), std::move(m_bindings), m_renderGroups,
                            m_groupShaderInfo, m_renderingTargetInfo, m_commandBuffer, std::move(m_onReady));
    star::core::waiter::one_shot::CreateDescriptorsOnEventPolicy<DescriptorRecipe>::Builder(m_bus)
        .setEventType(m_eventType)
        .setPolicy(std::move(recipe))
        .buildShared();
}
} // namespace star::core::renderer