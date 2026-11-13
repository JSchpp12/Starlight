#pragma once

#include "StarShaderInfo.hpp"
#include <star_common/Handle.hpp>

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include <memory>
#include <optional>
#include <set>
#include <vector>

namespace star::core::device
{
class DeviceContext;
}

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

    StarMaterial(glm::vec4 surfaceColor, glm::vec4 highlightColor, glm::vec4 ambient, glm::vec4 diffuse,
                 glm::vec4 specular, int shiny)
        : surfaceColor(surfaceColor), highlightColor(highlightColor), ambient(ambient), diffuse(diffuse),
          specular(specular), shinyCoefficient(shiny) {};

    StarMaterial() = default;
    virtual ~StarMaterial() = default;

    /// <summary>
    /// Entry point for rendering preparations. Ensure the base is called for every child instance
    /// </summary>
    /// <param name="device"></param>
    virtual void prepRender(core::device::DeviceContext &context, uint8_t numFramesInFlight,
                            star::StarShaderInfo::Builder frameBuilder, star::Handle commandBuffer);

    virtual void cleanupRender(core::device::DeviceContext &context);

    virtual void bind(vk::CommandBuffer &commandBuffer, vk::PipelineLayout pipelineLayout, int swapChainImageIndex,
                      uint32_t firstSetIndex = 0);

    /// @brief Replace one of this material's bound resources and mark its descriptor sets for rebuild.
    /// @details Takes the same arguments as StarShaderInfo::setNewResource so no material type needs an override.
    /// Recording the transfer wait, the one-shot setup, and retiring the old resource stay with the caller/service.
    void setResourceHandle(size_t setIndex, size_t bindingIndex, StarShaderInfo::ShaderInfo newHandle,
                           std::optional<size_t> frameInFlightIndex = std::nullopt);

    /// @brief Current bound resource handle (for retirement).
    Handle getResourceHandle() const
    {
        return m_resourceHandle;
    }

    /// @brief Copy of the resource currently bound at (setIndex, bindingIndex).
    /// @details `frameInFlightIndex == nullopt` reads frame 0; slots for all frames hold the same resource after a
    /// swap. Returns nullopt when the material has no built shader info or the slot is empty.
    [[nodiscard]] std::optional<StarShaderInfo::ShaderInfo> getResourceInfo(
        size_t setIndex, size_t bindingIndex, std::optional<size_t> frameInFlightIndex = std::nullopt) const;

    bool isKnownToBeReady(uint8_t swapChainImageIndex);

    /// Add the descriptor types to be used in this material to the provided layout builder. The layout builder should
    /// already contain parent descriptor information.
    virtual void addDescriptorSetLayoutsTo(star::StarDescriptorSetLayout::Builder &frameBuilder) const {};

    virtual std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(int numFramesInFlight) const;

    std::set<std::pair<vk::Semaphore, vk::PipelineStageFlags>> getDataSemaphores(uint8_t frameInFlightIndex) const;

  protected:
    std::unique_ptr<StarShaderInfo> shaderInfo;
    /// @brief Last resource bound through setResourceHandle; used by the MaterialService for retirement.
    Handle m_resourceHandle;

    /// @brief Create descriptor sets which will be used when this material is bound. Make sure that all global sets are
    virtual std::unique_ptr<StarShaderInfo> buildShaderInfo(core::device::DeviceContext &device,
                                                            uint8_t numFramesInFlight, StarShaderInfo::Builder builder);
};
} // namespace star