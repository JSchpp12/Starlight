#pragma once

#include "StarScene.hpp"
#include "core/SystemContext.hpp"

#include <star_common/FrameTracker.hpp>
#include <memory>
#include <string>
#include <vector>

namespace star
{
class StarApplication
{
  public:
    StarApplication() = default;
    virtual ~StarApplication() = default;

    virtual void init() = 0;

    virtual void frameUpdate(core::SystemContext &systemContext) = 0;

    virtual void shutdown(core::device::DeviceContext &context) = 0;

    // virtual void onKeyPress(int key, int scancode, int mods) override {};

    // virtual void onKeyRelease(int key, int scancode, int mods) override {};

    // virtual void onMouseMovement(double xpos, double ypos) override {};

    // virtual void onMouseButtonAction(int button, int action, int mods) override {};

    // virtual void onScroll(double xoffset, double yoffset) override {};

    virtual std::shared_ptr<StarScene> loadScene(core::device::DeviceContext &context,
                                                 const uint8_t &numFramesInFlight) = 0;

  protected:
};
} // namespace star
