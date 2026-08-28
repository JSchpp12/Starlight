#pragma once

#include "StarShaderInfo.hpp"
#include "core/device/DeviceContext.hpp"
#include "core/renderer/FrameData.hpp"
#include "core/renderer/RenderingTargetInfo.hpp"
#include "systems/StarRenderGroup.hpp"

#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>

#include <functional>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace star::common
{
class EventBus;
}

namespace star::core::renderer
{
Handle shaderInfoHandle(std::string_view name);

class DescriptorRecipe
{
  public:
    /// @brief A registered StarShaderInfo output target. Each target owns a contiguous range of pipeline descriptor
    /// sets starting at `baseSet`; `addBinding()` uses pipeline-global set numbers that the recipe demaps to the
    /// target's local set index (globalSet - baseSet). This makes the `set` argument match `layout(set = N, ...)` in
    /// GLSL verbatim and keeps the pipeline-set assignment in one place instead of being split across the recipe and
    /// the pipeline-layout assembler.
    struct ShaderInfoTarget
    {
        Handle handle;
        std::unique_ptr<StarShaderInfo> *out;
        uint32_t baseSet;
    };

    struct Binding
    {
        Handle shaderInfo; // which StarShaderInfo target this binding builds into
        uint32_t set;      // pipeline-global set number (matches `set = N` in GLSL)
        std::shared_ptr<FrameData> source;
        Handle role;      // role within source
        uint32_t binding; // binding index within the set
        vk::DescriptorType type;
        vk::ShaderStageFlags stage;
    };

    class Builder
    {
      public:
        Builder(common::EventBus &bus, core::device::DeviceContext &context, std::string_view eventName);

        /// @brief Register a StarShaderInfo output target and declare the first pipeline descriptor-set number it owns.
        Builder &setShaderInfoOut(Handle shaderInfo, std::unique_ptr<StarShaderInfo> *out, uint32_t baseSet);

        /// @brief Add a binding to the current target (set by the last setShaderInfoOut()). `set` is the
        /// pipeline-global set number -- it matches `layout(set = N, ...)` in the shader.
        Builder &addBinding(std::shared_ptr<FrameData> source, uint32_t set, uint32_t binding, Handle role,
                            vk::DescriptorType type, vk::ShaderStageFlags stage);

        /// Optional: notify render groups with the built layout so they can
        /// assemble their pipeline layout.
        Builder &setRenderGroups(Handle groupShaderInfo, std::vector<StarRenderGroup> *groups, RenderingTargetInfo info,
                                 Handle commandBuffer);

        /// Optional: generic post-build callback fired after all StarShaderInfo are built
        Builder &setOnShaderInfoReady(std::function<void(core::device::DeviceContext &)> onReady);

        void build();

      private:
        common::EventBus &m_bus;
        uint16_t m_eventType;
        core::device::DeviceContext *m_context{nullptr};
        std::vector<ShaderInfoTarget> m_shaderInfoTargets;
        std::vector<Binding> m_bindings;
        Handle m_currentShaderInfo{}; // implied target for addBinding()
        std::vector<StarRenderGroup> *m_renderGroups{nullptr};
        Handle m_groupShaderInfo{};
        RenderingTargetInfo m_renderingTargetInfo;
        Handle m_commandBuffer{};
        std::function<void(core::device::DeviceContext &)> m_onReady;
    };

    DescriptorRecipe() = default;
    int operator()();

  private:
    friend class Builder;

    DescriptorRecipe(core::device::DeviceContext *context, std::vector<ShaderInfoTarget> shaderInfoTargets,
                     std::vector<Binding> bindings, std::vector<StarRenderGroup> *renderGroups, Handle groupShaderInfo,
                     RenderingTargetInfo renderingTargetInfo, Handle commandBuffer,
                     std::function<void(core::device::DeviceContext &)> onReady);

    core::device::DeviceContext *m_context{nullptr};
    std::vector<ShaderInfoTarget> m_shaderInfoTargets;
    std::vector<Binding> m_bindings;
    std::vector<StarRenderGroup> *m_renderGroups{nullptr};
    Handle m_groupShaderInfo{};
    RenderingTargetInfo m_renderingTargetInfo;
    Handle m_commandBuffer{};
    std::function<void(core::device::DeviceContext &)> m_onReady;
};
} // namespace star::core::renderer
