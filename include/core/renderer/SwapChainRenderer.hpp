#pragma once

#include "DeviceContext.hpp"
#include "Renderer.hpp"

#include <memory>
#include <vulkan/vulkan.hpp>

namespace star::core::renderer
{
class SwapChainRenderer : public star::core::renderer::Renderer
{
  public:
    SwapChainRenderer(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                      std::vector<std::shared_ptr<StarObject>> objects, std::vector<std::shared_ptr<Light>> lights,
                      std::vector<Handle> &cameraInfoBuffers, const StarWindow &window);

    SwapChainRenderer(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                      std::vector<std::shared_ptr<StarObject>> objects, std::vector<std::shared_ptr<Light>> lights,
                      std::shared_ptr<StarCamera> camera, const StarWindow &window);

    SwapChainRenderer(const SwapChainRenderer &other) = delete;

    virtual ~SwapChainRenderer();

    virtual void prepRender(core::device::DeviceContext &device, const vk::Extent2D &swapChainExtent,
                            const uint8_t &numFramesInFlight) override;

    void submitPresentation(const int &frameIndexToBeDrawn, const vk::Semaphore *mainGraphicsDoneSemaphore);

    void pollEvents();

    void triggerScreenshot(const std::string &path) {
        // this->screenshotCommandBuffer->takeScreenshot(path);
    };

    vk::Extent2D getMainExtent() const
    {
        return *this->swapChainExtent;
    }
    uint8_t getFrameToBeDrawn() const
    {
        return this->currentFrameInFlightCounter;
    }

    vk::Fence getframeStatusFlag(const uint8_t &frameIndex)
    {
        assert(frameIndex < this->imagesInFlight.size() && "Out of range.");
        return this->imagesInFlight[frameIndex];
    }

  protected:
    const StarWindow &window;
    core::device::DeviceContext &device;

    // tracker for which frame is being processed of the available permitted frames
    uint8_t currentFrameInFlightCounter = 0, previousFrame = 0, numFramesInFlight = 0;

    uint32_t currentSwapChainImageIndex = 0;

    bool frameBufferResized =
        false; // explicit declaration of resize, used if driver does not trigger VK_ERROR_OUT_OF_DATE

    // more swapchain info
    vk::SwapchainKHR swapChain = vk::SwapchainKHR();

    std::unique_ptr<vk::Extent2D> swapChainExtent = std::unique_ptr<vk::Extent2D>();

    // Sync obj storage
    std::vector<vk::Fence> inFlightFences;
    std::vector<vk::Fence> imagesInFlight;
    std::vector<vk::Semaphore> imageAvailableSemaphores;
    std::vector<vk::Semaphore> imageAcquireSemaphores;

    virtual star::core::device::manager::ManagerCommandBuffer::Request getCommandBufferRequest() override;

    // std::unique_ptr<ScreenshotBuffer> screenshotCommandBuffer = nullptr;
    std::unique_ptr<std::string> screenshotPath = nullptr;

    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats) const;

    vk::Format getColorAttachmentFormat(core::device::DeviceContext &context) const override;

    // Look through givent present modes and pick the "best" one
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities);

    void prepareForSubmission(const int &frameIndexToBeDrawn);

    vk::Semaphore submitBuffer(StarCommandBuffer &buffer, const int &frameIndexToBeDrawn,
                               std::vector<vk::Semaphore> *previousCommandBufferSemaphores, std::vector<vk::Semaphore> dataSemaphores, std::vector<vk::PipelineStageFlags> dataWaitPoints);

    virtual std::vector<std::unique_ptr<StarTextures::Texture>> createRenderToImages(
        core::device::DeviceContext &device, const int &numFramesInFlight) override;

    virtual vk::RenderingAttachmentInfo prepareDynamicRenderingInfoColorAttachment(
        const int &frameInFlightIndex) override;

    virtual void recordCommandBuffer(vk::CommandBuffer &buffer, const uint8_t &frameIndexToBeDrawn, const uint64_t &frameIndex) override;

    /// <summary>
    /// If the swapchain is no longer compatible, it must be recreated.
    /// </summary>
    virtual void recreateSwapChain();

    void cleanupSwapChain();

    /// <summary>
    /// Create semaphores that are going to be used to sync rendering and presentation queues
    /// </summary>
    static std::vector<vk::Semaphore> CreateSemaphores(core::device::DeviceContext &device, const int &numToCreate);

    /// <summary>
    /// Fences are needed for CPU-GPU sync. Creates these required objects
    /// </summary>
    void createFences();

    /// <summary>
    /// Create tracking information in order to link fences with the swap chain images using
    /// </summary>
    void createFenceImageTracking();

    /// <summary>
    /// Create a swap chain that will be used in rendering images
    /// </summary>
    virtual void createSwapChain();
};
} // namespace star::core::renderer