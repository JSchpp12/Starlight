#pragma once

#include "starlight/core/renderer/RendererBase.hpp"

#include <star_common/IEvent.hpp>
#include <string_view>
namespace star::event
{
inline constexpr std::string_view GetRegisterMainGraphicsRendererTypeName = "star::event::RegisterMainGraphicsRenderer";

class RegisterMainGraphicsRenderer : public common::IEvent
{
  public:
    explicit RegisterMainGraphicsRenderer(core::renderer::RendererBase *renderer);

    const core::renderer::RendererBase *getRenderer() const
    {
        return m_renderer;
    }

  private:
    core::renderer::RendererBase *m_renderer = nullptr;
};
} // namespace star::event