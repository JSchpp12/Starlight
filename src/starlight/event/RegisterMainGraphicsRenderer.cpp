#include "starlight/event/RegisterMainGraphicsRenderer.hpp"

#include <star_common/HandleTypeRegistry.hpp>

namespace star::event
{
RegisterMainGraphicsRenderer::RegisterMainGraphicsRenderer(core::renderer::RendererBase *renderer)
    : common::IEvent(common::HandleTypeRegistry::instance().registerType(GetRegisterMainGraphicsRendererTypeName())),
      m_renderer(renderer)
{
}
} // namespace star::event