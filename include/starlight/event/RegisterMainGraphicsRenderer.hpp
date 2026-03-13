#pragma once

#include "starlight/core/renderer/RendererBase.hpp"

#include <star_common/IEvent.hpp>
#include <string_view>
namespace star::event
{
namespace register_main_graphics_renderer
{
inline constexpr const char *GetUniqueTypeName()
{
    return "eRMainGfx";
}
} // namespace register_main_graphics_renderer

class RegisterMainGraphicsRenderer : public common::IEvent
{
  public:
    static constexpr std::string_view GetUniqueTypeName()
    {
        return register_main_graphics_renderer::GetUniqueTypeName();
    }

    explicit RegisterMainGraphicsRenderer(core::renderer::RendererBase *renderer);

    const core::renderer::RendererBase *getRenderer() const
    {
        return m_renderer;
    }

  private:
    core::renderer::RendererBase *m_renderer = nullptr;
};
} // namespace star::event