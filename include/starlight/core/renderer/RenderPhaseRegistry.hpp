#pragma once

#include <star_common/Handle.hpp>

namespace star::core::renderer
{
class RenderPhase;

class RenderPhaseRegistry
{
  public:
    virtual ~RenderPhaseRegistry() = default;

    virtual RenderPhase *getPhase(const Handle &handle) noexcept = 0;
    virtual const RenderPhase *getPhase(const Handle &handle) const noexcept = 0;
};
} // namespace star::core::renderer