#pragma once

#include "Light.hpp"
#include "TransferRequest_Buffer.hpp"

#include <glm/glm.hpp>
#include <vector>

namespace star::TransferRequest
{
class LightInfo : public Buffer
{
  public:
    LightInfo(const uint32_t &numLights, const uint32_t &graphicsQueueFamilyIndex)
        : graphicsQueueFamilyIndex(graphicsQueueFamilyIndex), m_numLights(numLights)
    {

    }

    virtual ~LightInfo(){}

    std::unique_ptr<StarBuffers::Buffer> createStagingBuffer(
        vk::Device &device, VmaAllocator &allocator) const override;

    std::unique_ptr<StarBuffers::Buffer> createFinal(vk::Device &device, VmaAllocator &allocator,
                                            const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(StarBuffers::Buffer &buffer) const override;

  protected:
    struct Info{
        uint32_t numLights; 
    };

    const uint32_t graphicsQueueFamilyIndex;
    uint32_t m_numLights; 
};
} // namespace star::TransferRequest