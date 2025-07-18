#pragma once

#include "StarObjectInstance.hpp"
#include "TransferRequest_Buffer.hpp"


#include <glm/glm.hpp>

namespace star::TransferRequest
{
class InstanceNormalInfo : public Buffer
{
  public:
    InstanceNormalInfo(const std::vector<std::unique_ptr<StarObjectInstance>> &objectInstances,
                       const uint32_t &graphicsQueueFamilyIndex, const vk::DeviceSize &minUniformBufferOffsetAlignment)
        : graphicsQueueFamilyIndex(graphicsQueueFamilyIndex),
          minUniformBufferOffsetAlignment(minUniformBufferOffsetAlignment)
    {
        for (auto &instance : objectInstances)
        {
            this->normalMatrixInfo.push_back(instance->getDisplayMatrix());
        }
    }

    std::unique_ptr<StarBuffers::Buffer> createStagingBuffer(
        vk::Device &device, VmaAllocator &allocator) const override;

    std::unique_ptr<StarBuffers::Buffer> createFinal(vk::Device &device, VmaAllocator &allocator,
                                            const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(StarBuffers::Buffer &buffer) const override;

  protected:
    const uint32_t graphicsQueueFamilyIndex;
    const vk::DeviceSize minUniformBufferOffsetAlignment;
    std::vector<glm::mat4> normalMatrixInfo = std::vector<glm::mat4>();
};
} // namespace star::TransferRequest