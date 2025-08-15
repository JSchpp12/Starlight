#include "DeviceContext.hpp"

star::core::SwapChainSupportDetails star::core::DeviceContext::getSwapchainSupportDetails()
{
    return m_device.getSwapchainSupport(*m_surface);
}