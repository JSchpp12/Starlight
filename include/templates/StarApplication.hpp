#pragma once

#include "Interactivity.hpp"
#include "StarScene.hpp"
#include "SwapChainRenderer.hpp"
#include "Time.hpp"

#include <GLFW/glfw3.h>
#include <memory>
#include <string>
#include <vector>

namespace star
{
class StarApplication : public Interactivity
{
  public:
    StarApplication()
    {
        this->registerInteractions();
    }
    virtual ~StarApplication() = default;

    void init(core::devices::DeviceContext &device, const StarWindow &window, const uint8_t &numFramesInFlight);

    void cleanup();

    virtual void onWorldUpdate(const uint32_t &frameInFlightIndex) override = 0;

    virtual void onKeyPress(int key, int scancode, int mods) override {};

    virtual void onKeyRelease(int key, int scancode, int mods) override {};

    virtual void onMouseMovement(double xpos, double ypos) override {};

    virtual void onMouseButtonAction(int button, int action, int mods) override {};

    virtual void onScroll(double xoffset, double yoffset) override {};

    std::shared_ptr<SwapChainRenderer> getPresentationRenderer()
    {
        return this->swapChainRenderer;
    }

    std::shared_ptr<StarScene> getInitialScene()
    {
        return this->scene;
    }

  protected:
    std::shared_ptr<StarScene> scene = nullptr;
    std::shared_ptr<SwapChainRenderer> swapChainRenderer = nullptr;

    virtual std::shared_ptr<StarScene> createInitialScene(core::devices::DeviceContext &device, const StarWindow &window,
                                                          const uint8_t &numFramesInFlight) = 0;

    virtual std::shared_ptr<SwapChainRenderer> createPresentationRenderer(core::devices::DeviceContext &device, const StarWindow &window,
                                                                          const uint8_t &numFramesInFlight);

    virtual void startup(core::devices::DeviceContext &device, const StarWindow &window, const uint8_t &numFramesInFlight) = 0; 
};
} // namespace star
