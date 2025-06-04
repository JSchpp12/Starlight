#include "StarApplication.hpp"

#include "BasicCamera.hpp"

void star::StarApplication::init(StarDevice &device, const StarWindow &window, const uint8_t &numFramesInFlight)
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
    StarDevice &device, const StarWindow &window, const uint8_t &numFramesInFlight)
{
    return std::shared_ptr<star::SwapChainRenderer>(
        new star::SwapChainRenderer(window, this->scene, device, numFramesInFlight));
}