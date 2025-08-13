#include "DeviceContext.hpp"

star::core::DeviceContext::DeviceContext(StarDevice&& device) : m_device(std::move(device))
{
}