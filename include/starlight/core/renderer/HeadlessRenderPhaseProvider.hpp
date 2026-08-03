#pragma once

#include "starlight/core/renderer/DefaultRenderPhaseProvider.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>

namespace star::core::renderer
{
class HeadlessRenderPhase;

class HeadlessRenderPhaseProvider : public DefaultRenderPhaseProvider
{
  public:
    HeadlessRenderPhaseProvider(core::device::DeviceContext &context, std::shared_ptr<std::vector<Light>> lights,
                                std::shared_ptr<StarCamera> camera, std::vector<std::shared_ptr<StarObject>> objects,
                                vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eFragmentShader);
    HeadlessRenderPhaseProvider(core::device::DeviceContext &context, std::vector<std::shared_ptr<StarObject>> objects,
                                std::shared_ptr<FrameData> frameData,
                                vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eFragmentShader);

    virtual ~HeadlessRenderPhaseProvider() = default;

    HeadlessRenderPhaseProvider(const HeadlessRenderPhaseProvider &) = delete;
    HeadlessRenderPhaseProvider &operator=(const HeadlessRenderPhaseProvider &) = delete;
    HeadlessRenderPhaseProvider(HeadlessRenderPhaseProvider &&) = default;
    HeadlessRenderPhaseProvider &operator=(HeadlessRenderPhaseProvider &&) = default;

    virtual std::unique_ptr<RenderPhase> build(core::device::DeviceContext &context, RenderPhaseRegistry &phases) override;

  protected:
    void prepareHeadlessPhase(HeadlessRenderPhase *phase, core::device::DeviceContext &context);
};
} // namespace star::core::renderer
