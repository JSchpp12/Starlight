#pragma once

#include "StarCamera.hpp"
#include "TransferRequest_Buffer.hpp"

#include <glm/glm.hpp>

namespace star::TransferRequest
{
class GlobalInfo : public Buffer
{
  public:
    GlobalInfo(const StarCamera &camera, const uint32_t &graphicsQueueIndex)
        : camera(camera), graphicsQueueIndex(graphicsQueueIndex)
    {
    }

    virtual ~GlobalInfo() = default;

    std::unique_ptr<StarBuffers::Buffer> createStagingBuffer(vk::Device &device,
                                                             VmaAllocator &allocator) const override;

    std::unique_ptr<StarBuffers::Buffer> createFinal(
        vk::Device &device, VmaAllocator &allocator,
        const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(StarBuffers::Buffer &buffer) const override;

  private:
    const StarCamera camera;
    const uint32_t graphicsQueueIndex;

    struct GlobalUniformBufferObject
    {
        alignas(16) glm::mat4 proj;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4
            inverseView; // used to extrapolate camera position, can be used to convert from camera to world space
    };
};
} // namespace star::TransferRequest