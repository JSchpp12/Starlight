#pragma once

#include "DescriptorModifier.hpp"
#include "DeviceContext.hpp"
#include "RenderResourceModifier.hpp"
#include "StarCommandBuffer.hpp"
#include "StarShader.hpp"
#include "StarShaderInfo.hpp"

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace star
{
class StarMaterial
{
  public:
    glm::vec4 surfaceColor{0.5f, 0.5f, 0.5f, 1.0f};
    glm::vec4 highlightColor{0.5f, 0.5f, 0.5f, 1.0f};
    glm::vec4 ambient{0.5f, 0.5f, 0.5f, 1.0f};
    glm::vec4 diffuse{0.5f, 0.5f, 0.5f, 1.0f};
    glm::vec4 specular{0.5f, 0.5f, 0.5f, 1.0f};
    int shinyCoefficient = 1;

    StarMaterial(const glm::vec4 &surfaceColor, const glm::vec4 &highlightColor, const glm::vec4 &ambient,
                 const glm::vec4 &diffuse, const glm::vec4 &specular, const int &shiny)
        : surfaceColor(surfaceColor), highlightColor(highlightColor), ambient(ambient), diffuse(diffuse),
          specular(specular), shinyCoefficient(shiny) {};

    StarMaterial() = default;

    virtual ~StarMaterial() = default;

    /// <summary>
    /// Entry point for rendering preparations. Ensure the base is called for every child instance
    /// </summary>
    /// <param name="device"></param>
    virtual void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                            star::StarShaderInfo::Builder frameBuilder);

    virtual void cleanupRender(core::device::DeviceContext &context);

    virtual void bind(vk::CommandBuffer &commandBuffer, vk::PipelineLayout pipelineLayout, int swapChainImageIndex);

    bool isKnownToBeReady(const uint8_t &swapChainImageIndex);

    /// Add the descriptor types to be used in this material to the provided layout builder. The layout builder should
    /// already contain parent descriptor information.
    virtual void addDescriptorSetLayoutsTo(star::StarDescriptorSetLayout::Builder &frameBuilder) const = 0;

    std::set<vk::Semaphore> getDependentHighPriorityDataSemaphores(const uint8_t &frameInFlightIndex) const; 

    virtual std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(
      const int &numFramesInFlight) const;

  protected:
    std::unique_ptr<StarShaderInfo> shaderInfo;

    /// @brief Create descriptor sets which will be used when this material is bound. Make sure that all global sets are
    virtual std::unique_ptr<StarShaderInfo> buildShaderInfo(core::device::DeviceContext &device,
                                                            const uint8_t &numFramesInFlight,
                                                            StarShaderInfo::Builder builder) = 0;

  private:
};
} // namespace star