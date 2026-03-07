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
        glm::vec4 position{1.0f};
        glm::vec4 direction{1.0f}; // direction in which the light is pointing
        glm::vec4 ambient{1.0f};
        glm::vec4 diffuse{1.0f};
        glm::vec4 specular{1.0f};
        // controls.x = inner cutoff diameter
        // controls.y = outer cutoff diameter
        glm::vec4 controls{0.0f}; // container for single float values
        // settings.x = enabled
        // settings.y = type
        glm::uvec4 settings{0}; // container for single uint values
        uint32_t luminance{1};
    };

    LightList(const std::vector<Light> &lights, const uint32_t &graphicsQueueFamilyIndex)
        : graphicsQueueFamilyIndex(graphicsQueueFamilyIndex)
    {
        for (size_t i = 0; i < lights.size(); ++i)
        {
            myLights.push_back(Light(lights[i]));
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