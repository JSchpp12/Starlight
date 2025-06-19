#pragma once

#include "CommandBufferModifier.hpp"
#include "SceneRenderer.hpp"
#include "ScreenshotCommandBuffer.hpp"

#include <vulkan/vulkan.hpp>
#include <memory>

namespace star
{
class SwapChainRenderer : public SceneRenderer
{
  public:
    SwapChainRenderer(const StarWindow &window, std::shared_ptr<StarScene> scene, StarDevice &device, const int &numFramesInFlight);

    virtual ~SwapChainRenderer();

    virtual void prepare(StarDevice &device, const vk::Extent2D &swapChainExtent,
                         const int &numFramesInFlight) override;

    void submitPresentation(const int &frameIndexToBeDrawn, const vk::Semaphore *mainGraphicsDoneSemaphore);

    void pollEvents();

    void triggerScreenshot(const std::string &path)
    {
        this->screenshotCommandBuffer->takeScreenshot(path);
    };

    vk::Extent2D getMainExtent() const
    {
        return *this->swapChainExtent;
    }
    int getFrameToBeDrawn() const
    {
        return this->currentFrameInFlightCounter;
    }

    vk::Fence getframeStatusFlag(const uint8_t frameIndex)
    {
        assert(frameIndex < this->imagesInFlight.size() && "Out of range.");
        return this->imagesInFlight[frameIndex];
    }

  protected:
    const StarWindow &window;
    StarDevice &device;

    // tracker for which frame is being processed of the available permitted frames
    uint32_t currentFrameInFlightCounter = 0;
    int previousFrame = 0;
    int numFramesInFlight;

    uint32_t currentSwapChainImageIndex = 0;

    bool frameBufferResized =
        false; // explicit declaration of resize, used if driver does not trigger VK_ERROR_OUT_OF_DATE

    // more swapchain info
    vk::SwapchainKHR swapChain;

    std::unique_ptr<vk::Format> swapChainImageFormat = std::unique_ptr<vk::Format>();
    std::unique_ptr<vk::Extent2D> swapChainExtent = std::unique_ptr<vk::Extent2D>();

    // Sync obj storage
    std::vector<vk::Fence> inFlightFences;
    std::vector<vk::Fence> imagesInFlight;
    std::vector<vk::Semaphore> imageAvailableSemaphores;
    std::vector<vk::Semaphore> imageAcquireSemaphores;

    std::unique_ptr<ScreenshotBuffer> screenshotCommandBuffer;
    std::unique_ptr<std::string> screenshotPath = nullptr;

    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats);

    // Look through givent present modes and pick the "best" one
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities);

    virtual void prepareForSubmission(const int &frameIndexToBeDrawn) override;

    vk::Semaphore submitBuffer(StarCommandBuffer &buffer, const int &frameIndexToBeDrawn,
                               std::vector<vk::Semaphore> mustWaitFor);

    std::optional<std::function<vk::Semaphore(StarCommandBuffer &, const int &, std::vector<vk::Semaphore>)>>
    getOverrideBufferSubmissionCallback() override;

    virtual std::vector<std::unique_ptr<StarTexture>> createRenderToImages(StarDevice &device,
                                                                           const int &numFramesInFlight) override;

    virtual vk::RenderingAttachmentInfo prepareDynamicRenderingInfoColorAttachment(
        const int &frameInFlightIndex) override;

    virtual void recordCommandBuffer(vk::CommandBuffer &buffer, const int &frameIndexToBeDrawn) override;

    /// <summary>
    /// If the swapchain is no longer compatible, it must be recreated.
    /// </summary>
    virtual void recreateSwapChain();

    void cleanupSwapChain();

    /// <summary>
    /// Create semaphores that are going to be used to sync rendering and presentation queues
    /// </summary>
    static std::vector<vk::Semaphore> CreateSemaphores(StarDevice &device, const int &numToCreate);

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

    // Inherited via SceneRenderer
    Command_Buffer_Order getCommandBufferOrder() override;
    vk::PipelineStageFlags getWaitStages() override;
    bool getWillBeSubmittedEachFrame() override;
    bool getWillBeRecordedOnce() override;
};
} // namespace star