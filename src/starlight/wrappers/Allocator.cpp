#include "Allocator.hpp"

#include "starlight/core/logging/LoggingFactory.hpp"

star::Allocator::Allocator(vk::Device device, vk::PhysicalDevice physicalDevice, vk::Instance instance)
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT &
                          VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT & VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT &
                          VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT & VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
    allocatorInfo.device = device;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.instance = instance;

    vmaCreateAllocator(&allocatorInfo, &this->allocator);
}

void star::Allocator::cleanupRender()
{
    if (!allocator)
        return;

#ifndef NDEBUG
    // Check if anything is still alive
    VmaTotalStatistics totals{};
    vmaCalculateStatistics(allocator, &totals);
    const uint32_t liveAllocs = totals.total.statistics.allocationCount;

    if (liveAllocs > 0)
    {
        char *stats = nullptr;
        vmaBuildStatsString(allocator, &stats, VK_TRUE);
        // Write to file (or your logger)
        std::ostringstream oss;
        oss << stats;

        star::core::logging::warning(oss.str());
        vmaFreeStatsString(allocator, stats);
    }

#endif

    vmaDestroyAllocator(allocator);
}