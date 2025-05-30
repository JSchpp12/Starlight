#pragma once

#include "StarBuffer.hpp"
#include "StarObjectInstance.hpp"
#include "TransferRequest_Buffer.hpp"


#include <glm/glm.hpp>

namespace star::TransferRequest
{
class InstanceModelInfo : public star::TransferRequest::Buffer
{
  public:
    InstanceModelInfo(const std::vector<std::unique_ptr<star::StarObjectInstance>> &objectInstances,
                      const uint32_t &graphicsQueueFamilyIndex, const vk::DeviceSize &minUniformBufferOffsetAlignment)
        : displayMatrixInfo(std::vector<glm::mat4>(objectInstances.size())),
          graphicsQueueFamilyIndex(graphicsQueueFamilyIndex),
          minUniformBufferOffsetAlignment(minUniformBufferOffsetAlignment)
    {
        for (int i = 0; i < objectInstances.size(); i++)
        {
            displayMatrixInfo[i] = objectInstances[i]->getDisplayMatrix();
        }
    }

    std::unique_ptr<StarBuffer> createStagingBuffer(vk::Device &device, VmaAllocator &allocator) const override;

    std::unique_ptr<StarBuffer> createFinal(vk::Device &device, VmaAllocator &allocator,
                                            const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(StarBuffer &buffer) const override;

  protected:
    const uint32_t graphicsQueueFamilyIndex;
    const vk::DeviceSize minUniformBufferOffsetAlignment;
    std::vector<glm::mat4> displayMatrixInfo;
};
} // namespace star::TransferRequest