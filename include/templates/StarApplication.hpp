#pragma once

#include "Interactivity.hpp"
#include "StarScene.hpp"
#include "Time.hpp"
#include "core/SystemContext.hpp"
#include "core/renderer/SwapChainRenderer.hpp"


#include <GLFW/glfw3.h>
#include <memory>
#include <string>
#include <vector>

namespace star
{
class StarApplication : public Interactivity
{
  public:
    StarApplication() = default;
    virtual ~StarApplication() = default;

    void init()
    {
        this->registerInteractions();
    }

    void cleanup();

    virtual void frameUpdate(star::core::SystemContext &context, const uint8_t &frameInFlightIndex) = 0;

    virtual void onKeyPress(int key, int scancode, int mods) override {};

    virtual void onKeyRelease(int key, int scancode, int mods) override {};

    virtual void onMouseMovement(double xpos, double ypos) override {};

    virtual void onMouseButtonAction(int button, int action, int mods) override {};

    virtual void onScroll(double xoffset, double yoffset) override {};

    virtual std::shared_ptr<StarScene> loadScene(core::device::DeviceContext &context, const StarWindow &window,
                                                 const uint8_t &numFramesInFlight) = 0;

  protected:
};
} // namespace star
