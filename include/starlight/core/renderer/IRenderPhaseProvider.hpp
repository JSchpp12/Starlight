#pragma once

#include "starlight/core/renderer/RenderPhaseRegistry.hpp"

#include <memory>

namespace star::core::device
{
class DeviceContext;
}

namespace star::core::renderer
{
class RenderPhase;

/// Builder for a RenderPhase. Concrete subclasses hold the per-phase setup
class IRenderPhaseProvider
{
  public:
    virtual ~IRenderPhaseProvider() = default;

    virtual std::unique_ptr<RenderPhase> build(core::device::DeviceContext &context, RenderPhaseRegistry &phases) = 0;
};
} // namespace star::core::renderer
