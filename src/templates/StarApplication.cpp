#include "StarApplication.hpp"

#include "BasicCamera.hpp"

void star::StarApplication::init(core::device::DeviceContext &device, const StarWindow &window, const uint8_t &numFramesInFlight)
{
    this->scene = createInitialScene(device, window, numFramesInFlight);
    this->swapChainRenderer = createPresentationRenderer(device, window, numFramesInFlight);
    startup(device, window, numFramesInFlight);
}

void star::StarApplication::cleanup()
{
    this->scene.reset();
    this->swapChainRenderer.reset();
}

std::shared_ptr<star::SwapChainRenderer> star::StarApplication::createPresentationRenderer(
    core::device::DeviceContext &device, const StarWindow &window, const uint8_t &numFramesInFlight)
{
    return std::make_shared<star::SwapChainRenderer>(this->scene, window, device, numFramesInFlight);
}