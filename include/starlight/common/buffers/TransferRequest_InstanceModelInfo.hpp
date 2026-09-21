#pragma once

#include "TransferRequest_Buffer.hpp"
#include "starlight/virtual/StarEntity.hpp"

#include <glm/glm.hpp>

namespace star::TransferRequest
{
class InstanceModelInfo : public star::TransferRequest::Buffer
{
  public:
    InstanceModelInfo(const std::vector<star::StarEntity> &objectInstances, const uint32_t &graphicsQueueFamilyIndex,
                      const vk::DeviceSize &minUniformBufferOffsetAlignment)
        : displayMatrixInfo(std::vector<glm::mat4>(objectInstances.size())),
          graphicsQueueFamilyIndex(graphicsQueueFamilyIndex),
          minUniformBufferOffsetAlignment(minUniformBufferOffsetAlignment)
    {
        for (size_t i = 0; i < objectInstances.size(); i++)
        {
            displayMatrixInfo[i] = objectInstances[i].getDisplayMatrix();
        }
    }

    std::unique_ptr<StarBuffers::Buffer> createStagingBuffer(core::device::StarDevice &device) const override;

    std::unique_ptr<StarBuffers::Buffer> createFinal(
        core::device::StarDevice &device, const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(StarBuffers::Buffer &buffer) const override;

  protected:
    std::vector<glm::mat4> displayMatrixInfo;
    const uint32_t graphicsQueueFamilyIndex;
    const vk::DeviceSize minUniformBufferOffsetAlignment;
};
} // namespace star::TransferRequest