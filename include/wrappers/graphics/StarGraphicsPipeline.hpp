#pragma once

#include "Compiler.hpp"
#include "DeviceContext.hpp"
#include "RenderingTargetInfo.hpp"
#include "StarPipeline.hpp"
#include "StarShader.hpp"
#include "VulkanVertex.hpp"

#include <vulkan/vulkan.hpp>

#include <iostream>

namespace star
{
class StarGraphicsPipeline : public StarPipeline
{
  public:
    struct PipelineConfigSettings
    {
        PipelineConfigSettings() = default;
        ~PipelineConfigSettings() = default;
        // no copy
        PipelineConfigSettings(const PipelineConfigSettings &) = delete;
        PipelineConfigSettings &operator=(const PipelineConfigSettings &) = delete;

        PipelineConfigSettings(PipelineConfigSettings &&) = default;
        PipelineConfigSettings &operator=(PipelineConfigSettings &&) = default;

        vk::Rect2D scissor;
        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        vk::PipelineRasterizationStateCreateInfo rasterizationInfo;
        vk::PipelineMultisampleStateCreateInfo multisampleInfo;
        vk::PipelineColorBlendAttachmentState colorBlendAttachment;
        vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
        vk::PipelineDepthStencilStateCreateInfo depthStencilInfo;
        std::vector<VkDynamicState> dynamicStateEnables;
        vk::PipelineDynamicStateCreateInfo dynamicStateInfo;
        vk::PipelineLayout pipelineLayout = nullptr;
        vk::Extent2D swapChainExtent;
        std::vector<vk::VertexInputBindingDescription> vertInputBindingDescription =
            std::vector<vk::VertexInputBindingDescription>{VulkanVertex::getBindingDescription()};
        uint32_t subpass = 0;
        RenderingTargetInfo renderingInfo;
    };

    StarGraphicsPipeline(StarShader vertShader, StarShader fragShader);

    StarGraphicsPipeline(StarShader vertShader, StarShader fragShader,
                         StarShader geomShader);

    virtual ~StarGraphicsPipeline() = default;
    // no copy
    StarGraphicsPipeline(const StarGraphicsPipeline &) = delete;
    StarGraphicsPipeline &operator=(const StarGraphicsPipeline &) = delete;
    StarGraphicsPipeline(StarGraphicsPipeline &&) = default;
    StarGraphicsPipeline &operator=(StarGraphicsPipeline &&) = default;

    virtual void bind(vk::CommandBuffer &commandBuffer) override;

    static void defaultPipelineConfigInfo(PipelineConfigSettings &configSettings, vk::Extent2D swapChainExtent,
                                          vk::PipelineLayout pipelineLayout, RenderingTargetInfo renderingInfo);

  protected:
    StarShader vertShader, fragShader;
    std::optional<StarShader> geomShader;
    PipelineConfigSettings configSettings;

    virtual vk::Pipeline buildPipeline(core::device::DeviceContext &context, vk::Extent2D swapChainExtent,
                                       vk::PipelineLayout pipelineLayout, RenderingTargetInfo renderingInfo) override;

    virtual std::vector<Handle> submitShaders(core::device::DeviceContext &context) override;
};
} // namespace star