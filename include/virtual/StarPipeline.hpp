#pragma once

#include "Handle.hpp"
#include "StarShader.hpp"
#include "VulkanVertex.hpp"
#include "core/device/StarDevice.hpp"
#include "core/renderer/RenderingTargetInfo.hpp"

#include <vulkan/vulkan.hpp>

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace star
{

class StarPipeline
{
  public:
    struct RenderResourceDepdencies
    {
        std::vector<std::pair<star::StarShader, std::unique_ptr<std::vector<uint32_t>>>> compiledShaders;
        core::renderer::RenderingTargetInfo renderingTargetInfo;
        vk::Extent2D swapChainExtent;
    };

    struct GraphicsPipelineConfigSettings
    {
        GraphicsPipelineConfigSettings() = default;
        ~GraphicsPipelineConfigSettings() = default;
        // no copy
        GraphicsPipelineConfigSettings(const GraphicsPipelineConfigSettings &) = delete;
        GraphicsPipelineConfigSettings &operator=(const GraphicsPipelineConfigSettings &) = delete;

        GraphicsPipelineConfigSettings(GraphicsPipelineConfigSettings &&) = default;
        GraphicsPipelineConfigSettings &operator=(GraphicsPipelineConfigSettings &&) = default;

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
        core::renderer::RenderingTargetInfo renderingInfo;
    };

    struct ComputePipelineConfigSettings
    {
        ComputePipelineConfigSettings() = default;
    };

#pragma region helpers
    /// <summary>
    /// Build a shader module with the provided SPIR-V source code
    /// </summary>
    /// <param name="sourceCode"></param>
    /// <returns></returns>
    static vk::ShaderModule CreateShaderModule(vk::Device &device, const std::vector<uint32_t> &sourceCode);
#pragma endregion

    StarPipeline() = default;
    StarPipeline(std::variant<GraphicsPipelineConfigSettings, ComputePipelineConfigSettings> configSettings,
                 vk::PipelineLayout pipelineLayout, std::vector<Handle> shaders);
    virtual ~StarPipeline();
    StarPipeline(const StarPipeline &) = delete;
    StarPipeline &operator=(const StarPipeline &) = delete;
    StarPipeline(StarPipeline &&other)
        : m_pipelineLayout(std::move(other.m_pipelineLayout)), m_shaders(std::move(other.m_shaders)),
          m_pipeline(std::move(other.m_pipeline)), m_configSettings(std::move(other.m_configSettings))
    {
    }
    StarPipeline &operator=(StarPipeline &&other)
    {
        if (this != &other)
        {
            m_pipelineLayout = std::move(other.m_pipelineLayout);
            m_shaders = std::move(other.m_shaders);
            m_pipeline = std::move(other.m_pipeline);
            m_configSettings = std::move(other.m_configSettings);
        }

        return *this;
    }
    bool operator!()
    {
        return m_shaders.size() == 0;
    }

    bool isRenderReady() const;

    void prepRender(core::device::StarDevice &device, RenderResourceDepdencies deps);

    void cleanupRender(core::device::StarDevice &device);

    void bind(vk::CommandBuffer &commandBuffer);

    const std::vector<Handle> &getShaders()
    {
        return m_shaders;
    }

  private:
    vk::PipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    std::variant<GraphicsPipelineConfigSettings, ComputePipelineConfigSettings> m_configSettings;
    std::vector<Handle> m_shaders = std::vector<Handle>();
    bool m_isGraphicsPipeline = false;

  protected:
    vk::Pipeline m_pipeline = VK_NULL_HANDLE;

    static vk::Pipeline BuildGraphicsPieline(vk::Device &device, const vk::PipelineLayout &pipelineLayout,
                                             const RenderResourceDepdencies &depdencies,
                                             GraphicsPipelineConfigSettings &pipelineSettings);

    static vk::Pipeline BuildComputePipeline(vk::Device &device, const vk::PipelineLayout &pipelineLayout,
                                             const RenderResourceDepdencies &depdencies,
                                             ComputePipelineConfigSettings &pipelineSettings);

    vk::Pipeline buildPipeline(core::device::StarDevice &device, const RenderResourceDepdencies &depdencies);

    bool isSame(StarPipeline &compPipe);

    bool hasSameShadersAs(StarPipeline &compPipe);

    static bool IsGraphicsPipeline(
        const std::variant<GraphicsPipelineConfigSettings, ComputePipelineConfigSettings> &settings);

    static void DefaultGraphicsPipelineConfigInfo(GraphicsPipelineConfigSettings &configSettings,
                                                  vk::Extent2D swapChainExtent, vk::PipelineLayout pipelineLayout,
                                                  core::renderer::RenderingTargetInfo renderingInfo);

    static void ProcessShaders(vk::Device &device, const RenderResourceDepdencies &deps,
                               vk::ShaderModule &vertShaderModule, vk::ShaderModule &fragShaderModule,
                               vk::ShaderModule &geoModule);
};
} // namespace star