#include "VulkanDispatch.hpp"

#include "starlight/core/Exceptions.hpp"

#include <exception>
#include <mutex>
#include <stdexcept>

#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC != 1
#error "VulkanDispatch.cpp requires VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1"
#endif

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace star::core
{

void InitializeVulkanDispatchLoader()
{
    static std::once_flag initializeDispatchLoaderOnce;

    std::call_once(initializeDispatchLoaderOnce, [] {
        try
        {
            static vk::detail::DynamicLoader loader;
            auto vkGetInstanceProcAddr = loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");

            if (!loader.success() || !vkGetInstanceProcAddr)
            {
                throw std::runtime_error("Failed to load the Vulkan dynamic library or resolve vkGetInstanceProcAddr");
            }

            VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
        }
        catch (const std::exception &ex)
        {
            STAR_THROW_CAUSE("Failed to initialize the Vulkan dispatch loader", ex);
        }
    });
}

void InitializeVulkanInstanceDispatchLoader(vk::Instance instance)
{
    static std::once_flag initializeDispatcherOnce; 
    std::call_once(initializeDispatcherOnce, [instance](){
        VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
    });
}
} // namespace star::core
