#include "SystemContext.hpp"

#include <cassert>

star::core::SystemContext::SystemContext(RenderingInstance *renderingInstance) : m_instance(renderingInstance)
{
}

star::Handle star::core::SystemContext::registerDevice(
    core::device::DeviceContext context)
{
    return m_contexts.insert(std::move(context));
}