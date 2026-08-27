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
    struct Binding
    {
        Handle shaderInfo; // which StarShaderInfo this binding builds into
        uint32_t set;      // which set within that StarShaderInfo
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

        /// @brief Register a StarShaderInfo sink and make it the current target for subsequent addBinding() calls.
        /// Calling again with the same handle just re-establishes it as current.
        Builder &setShaderInfoOut(Handle shaderInfo, std::unique_ptr<StarShaderInfo> *out);

        /// @brief Add a binding to the current shader info (set by the last setShaderInfoOut())
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
        std::vector<std::pair<Handle, std::unique_ptr<StarShaderInfo> *>> m_shaderInfoOuts;
        std::vector<Binding> m_bindings;
        Handle m_currentShaderInfo{}; // implied shaderInfo for addBinding()
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

    DescriptorRecipe(core::device::DeviceContext *context,
                     std::vector<std::pair<Handle, std::unique_ptr<StarShaderInfo> *>> shaderInfoOuts,
                     std::vector<Binding> bindings, std::vector<StarRenderGroup> *renderGroups, Handle groupShaderInfo,
                     RenderingTargetInfo renderingTargetInfo, Handle commandBuffer,
                     std::function<void(core::device::DeviceContext &)> onReady);

    core::device::DeviceContext *m_context{nullptr};
    std::vector<std::pair<Handle, std::unique_ptr<StarShaderInfo> *>> m_shaderInfoOuts;
    std::vector<Binding> m_bindings;
    std::vector<StarRenderGroup> *m_renderGroups{nullptr};
    Handle m_groupShaderInfo{};
    RenderingTargetInfo m_renderingTargetInfo;
    Handle m_commandBuffer{};
    std::function<void(core::device::DeviceContext &)> m_onReady;
};
} // namespace star::core::renderer