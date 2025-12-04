#pragma once

#include "DefaultRenderer.hpp"
#include "core/device/DeviceContext.hpp"

#include <memory>
#include <vulkan/vulkan.hpp>

namespace star::core::renderer
{
class SwapChainRenderer : public DefaultRenderer
{
  public:
    SwapChainRenderer(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                      std::vector<std::shared_ptr<StarObject>> objects, std::shared_ptr<std::vector<Light>> lights,
                      std::shared_ptr<StarCamera> camera, const StarWindow &window);

    SwapChainRenderer(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                      std::vector<std::shared_ptr<StarObject>> objects,
                      std::shared_ptr<ManagerController::RenderResource::Buffer> lightData,
                      std::shared_ptr<ManagerController::RenderResource::Buffer> lightListData,
                      std::shared_ptr<ManagerController::RenderResource::Buffer> cameraData, const StarWindow &window);

    SwapChainRenderer(const SwapChainRenderer &other) noexcept = delete;
    SwapChainRenderer &operator=(const SwapChainRenderer &) noexcept = delete;

    virtual ~SwapChainRenderer() = default;

    virtual void prepRender(core::device::DeviceContext &device, const uint8_t &numFramesInFlight) override;

    virtual void cleanupRender(core::device::DeviceContext &device) override;

    virtual void frameUpdate(core::device::DeviceContext &context, const uint8_t &frameInFlightIndex) override;

    void submitPresentation(const int &frameIndexToBeDrawn, const vk::Semaphore *mainGraphicsDoneSemaphore);

    void pollEvents();

    vk::Extent2D getMainExtent() const
    {
        return *this->swapChainExtent;
    }
    uint8_t getFrameToBeDrawn() const
    {
        return this->currentFrameInFlightCounter;
    }

    std::vector<Handle> &getDoneSemaphores()
    {
        return imageAvailableSemaphores;
    }
    std::vector<Handle> getDoneSemaphores() const
    {
        return imageAvailableSemaphores;
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
    std::vector<Handle> inFlightFences, imagesInFlight;
    std::vector<Handle> imageAvailableSemaphores, imageAcquireSemaphores;
    std::vector<Handle> graphicsDoneSemaphoresExternalUse; /// These are guaranteed to match with the current frame in
                                                           /// flight for other command buffers to reference

    virtual star::core::device::manager::ManagerCommandBuffer::Request getCommandBufferRequest() override;

    // std::unique_ptr<ScreenshotBuffer> screenshotCommandBuffer = nullptr;
    std::unique_ptr<std::string> screenshotPath = nullptr;

    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats) const;

    bool doesSwapChainSupportTransferOperations(core::device::DeviceContext &context) const;

    vk::Format getColorAttachmentFormat(core::device::DeviceContext &context) const override;

    // Look through givent present modes and pick the "best" one
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities);

    void prepareForSubmission(const int &frameIndexToBeDrawn);

    vk::Semaphore submitBuffer(StarCommandBuffer &buffer, const int &frameIndexToBeDrawn,
                               std::vector<vk::Semaphore> *previousCommandBufferSemaphores,
                               std::vector<vk::Semaphore> dataSemaphores,
                               std::vector<vk::PipelineStageFlags> dataWaitPoints,
                               std::vector<std::optional<uint64_t>> previousSignaledValues);

    virtual std::vector<StarTextures::Texture> createRenderToImages(core::device::DeviceContext &device,
                                                                    const uint8_t &numFramesInFlight) override;

    virtual vk::RenderingAttachmentInfo prepareDynamicRenderingInfoColorAttachment(
        const int &frameInFlightIndex) override;

    virtual void recordCommandBuffer(vk::CommandBuffer &buffer, const uint8_t &frameIndexToBeDrawn,
                                     const uint64_t &frameIndex) override;

    /// <summary>
    /// If the swapchain is no longer compatible, it must be recreated.
    /// </summary>
    virtual void recreateSwapChain();

    void cleanupSwapChain(core::device::DeviceContext &context);

    /// <summary>
    /// Create semaphores that are going to be used to sync rendering and presentation queues
    /// </summary>
    static std::vector<Handle> CreateSemaphores(core::device::DeviceContext &context, const uint8_t &numToCreate,
                                                const bool &isTimeline);

    /// <summary>
    /// Fences are needed for CPU-GPU sync. Creates these required objects
    /// </summary>
    void createFences(core::device::DeviceContext &context);

    /// <summary>
    /// Create tracking information in order to link fences with the swap chain images using
    /// </summary>
    void createFenceImageTracking();

    /// <summary>
    /// Create a swap chain that will be used in rendering images
    /// </summary>
    void createSwapChain(core::device::DeviceContext &context);

    void prepareRenderingContext(core::device::DeviceContext &context);

    void addSemaphoresToRenderingContext(core::device::DeviceContext &context);

    void addFencesToRenderingContext(core::device::DeviceContext &context);

    std::vector<vk::ImageMemoryBarrier2> getImageBarriersForThisFrame();
};
} // namespace star::core::renderer