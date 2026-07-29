#pragma once

#include <memory>

namespace star::core::device
{
class DeviceContext;
}

namespace star::core::renderer
{
class RenderPhase;

/// Builder for a RenderPhase. Concrete subclasses hold the per-phase setup
/// recipe (e.g. the swapchain handle + windowing context for a presented phase)
/// and produce the runtime RenderPhase in build(). This is cold-path setup --
/// run once at prep and again on resize -- so a virtual here is acceptable.
class IRenderPhaseProvider
{
  public:
    virtual ~IRenderPhaseProvider() = default;

    virtual std::unique_ptr<RenderPhase> build(core::device::DeviceContext &context) = 0;
};
} // namespace star::core::renderer