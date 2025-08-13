#pragma once


#include "StarDevice.hpp"
#include "TaskManager.hpp"


#include <vulkan/vulkan.hpp>

namespace star::core{
    class DeviceContext{
        public:
        DeviceContext(StarDevice&& device);
        ~DeviceContext() = default;

        DeviceContext(const DeviceContext&) = delete;
        DeviceContext& operator=(const DeviceContext&) = delete;

        private:
        StarDevice m_device;
        job::TaskManager m_manager;

    };
}