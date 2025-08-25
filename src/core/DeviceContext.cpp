#include "DeviceContext.hpp"

star::core::SwapChainSupportDetails star::core::devices::DeviceContext::getSwapchainSupportDetails()
{
    return m_device.getSwapchainSupport(*m_surface);
}