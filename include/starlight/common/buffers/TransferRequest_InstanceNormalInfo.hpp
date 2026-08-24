#pragma once

#include "TransferRequest_Buffer.hpp"
#include "starlight/virtual/StarEntity.hpp"

#include <glm/glm.hpp>

namespace star::TransferRequest
{
class InstanceNormalInfo : public Buffer
{
  public:
    InstanceNormalInfo(const std::vector<StarEntity> &objectInstances, const uint32_t &graphicsQueueFamilyIndex,
                       const vk::DeviceSize &minUniformBufferOffsetAlignment)
        : graphicsQueueFamilyIndex(graphicsQueueFamilyIndex),
          minUniformBufferOffsetAlignment(minUniformBufferOffsetAlignment),
          normalMatrixInfo(std::vector<glm::mat4>(objectInstances.size()))
    {
        for (size_t i = 0; i < objectInstances.size(); i++)
        {
            normalMatrixInfo[i] = objectInstances[i].getDisplayMatrix();
        }
    }

    std::unique_ptr<StarBuffers::Buffer> createStagingBuffer(core::device::StarDevice &device) const override;

    std::unique_ptr<StarBuffers::Buffer> createFinal(
        core::device::StarDevice &device, const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(StarBuffers::Buffer &buffer) const override;

  protected:
    const uint32_t graphicsQueueFamilyIndex;
    const vk::DeviceSize minUniformBufferOffsetAlignment;
    std::vector<glm::mat4> normalMatrixInfo = std::vector<glm::mat4>();
};
} // namespace star::TransferRequest