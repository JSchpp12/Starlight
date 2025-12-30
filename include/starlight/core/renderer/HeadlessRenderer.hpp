#pragma once

#include <starlight/core/renderer/DefaultRenderer.hpp>

namespace star::core::renderer
{
class HeadlessRenderer : public star::core::renderer::DefaultRenderer
{
  public:
    HeadlessRenderer() = default;
    
    virtual ~HeadlessRenderer() = default;

  private:
};
} // namespace star::core::renderer