#pragma once

#include "Light.hpp"
#include "TransferRequest_Buffer.hpp"

namespace star::TransferRequest
{
class LightList : public Buffer
{
  public:
    struct LightBufferObject
    {
        glm::vec4 position = glm::vec4(1.0f);
        glm::vec4 direction = glm::vec4(1.0f); // direction in which the light is pointing
        glm::vec4 ambient = glm::vec4(1.0f);
        glm::vec4 diffuse = glm::vec4(1.0f);
        glm::vec4 specular = glm::vec4(1.0f);
        // controls.x = inner cutoff diameter
        // controls.y = outer cutoff diameter
        glm::vec4 controls = glm::vec4(0.0f); // container for single float values
        // settings.x = enabled
        // settings.y = type
        glm::uvec4 settings = glm::uvec4(0); // container for single uint values
    };

    LightList(const std::vector<std::shared_ptr<Light>> &lights, const uint32_t &graphicsQueueFamilyIndex)
        : graphicsQueueFamilyIndex(graphicsQueueFamilyIndex)
    {
        for (size_t i = 0; i < lights.size(); ++i)
        {
            myLights.push_back(Light(*lights[i].get()));
        }
    }

    std::unique_ptr<StarBuffers::Buffer> createStagingBuffer(vk::Device &device,
                                                             VmaAllocator &allocator) const override;

    std::unique_ptr<StarBuffers::Buffer> createFinal(
        vk::Device &device, VmaAllocator &allocator,
        const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(StarBuffers::Buffer &buffer) const override;

  private:
    const uint32_t graphicsQueueFamilyIndex;
    std::vector<Light> myLights;
};
} // namespace star::TransferRequest