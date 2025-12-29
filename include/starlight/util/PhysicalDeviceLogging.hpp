
#pragma once

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>   // for VK_VERSION_* macros
#include <vulkan/vulkan.hpp> // Vulkan-Hpp

namespace star::log
{

inline std::string formatBytes(uint64_t bytes)
{
    const char *units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    int unitIdx = 0;
    double val = static_cast<double>(bytes);
    while (val >= 1024.0 && unitIdx < 4)
    {
        val /= 1024.0;
        ++unitIdx;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(val < 10.0 ? 2 : 0) << val << " " << units[unitIdx];
    return oss.str();
}

inline std::string formatApiVersion(uint32_t v)
{
    std::ostringstream oss;
    oss << VK_VERSION_MAJOR(v) << "." << VK_VERSION_MINOR(v) << "." << VK_VERSION_PATCH(v);
    return oss.str();
}

inline const char *deviceTypeToStr(vk::PhysicalDeviceType t)
{
    switch (t)
    {
    case vk::PhysicalDeviceType::eOther:
        return "Other";
    case vk::PhysicalDeviceType::eIntegratedGpu:
        return "Integrated GPU";
    case vk::PhysicalDeviceType::eDiscreteGpu:
        return "Discrete GPU";
    case vk::PhysicalDeviceType::eVirtualGpu:
        return "Virtual GPU";
    case vk::PhysicalDeviceType::eCpu:
        return "CPU";
    default:
        return "Unknown";
    }
}

inline std::string vendorHint(uint32_t vendorId)
{
    // Optional friendly vendor guess. PCI IDs aren't guaranteed but common ones are useful.
    switch (vendorId)
    {
    case 0x10DE:
        return " (NVIDIA)";
    case 0x1002: // AMD (ATI)
    case 0x1022:
        return " (AMD)";
    case 0x8086:
        return " (Intel)";
    case 0x13B5:
        return " (Arm)";
    default:
        return "";
    }
}

inline std::string summarizeProperties(const vk::PhysicalDeviceProperties &p)
{
    std::ostringstream oss;
    oss << "Device: " << p.deviceName << "\n"
        << "Type: " << deviceTypeToStr(p.deviceType) << "\n"
        << "API Version: " << formatApiVersion(p.apiVersion) << "\n"
        << "Driver Version: 0x" << std::hex << p.driverVersion << std::dec << "\n"
        << "Vendor ID: 0x" << std::hex << p.vendorID << std::dec << vendorHint(p.vendorID) << "\n"
        << "Device ID: 0x" << std::hex << p.deviceID << std::dec << "\n";

    const auto &lim = p.limits;
    oss << "Limits:\n"
        << "  Max image dimension 2D: " << lim.maxImageDimension2D << "\n"
        << "  Max uniform buffer range: " << lim.maxUniformBufferRange << " bytes\n"
        << "  Max storage buffer range: " << lim.maxStorageBufferRange << " bytes\n"
        << "  Max bound descriptor sets: " << lim.maxBoundDescriptorSets << "\n"
        << "  Max descriptor set uniform buffers: " << lim.maxDescriptorSetUniformBuffers << "\n"
        << "  Max descriptor set storage buffers: " << lim.maxDescriptorSetStorageBuffers << "\n"
        << "  Max per-stage descriptor samplers: " << lim.maxPerStageDescriptorSamplers << "\n"
        << "  BufferImageGranularity: " << lim.bufferImageGranularity << " bytes\n"
        << "  SparseResidency: "
        << (lim.sparseAddressSpaceSize ? "Supported (size " + formatBytes(lim.sparseAddressSpaceSize) + ")"
                                       : "Unknown/Disabled")
        << "\n";
    return oss.str();
}

inline std::string summarizeQueueFamilies(const std::vector<vk::QueueFamilyProperties> &families)
{
    std::ostringstream oss;
    oss << "Queue Families (" << families.size() << "):\n";
    for (size_t i = 0; i < families.size(); ++i)
    {
        const auto &q = families[i];
        oss << "  [" << i << "] count=" << q.queueCount << " | flags=";

        const auto f = q.queueFlags;
        bool first = true;
        auto add = [&](const char *s) {
            oss << (first ? "" : ", ") << s;
            first = false;
        };
        if (f & vk::QueueFlagBits::eGraphics)
            add("Graphics");
        if (f & vk::QueueFlagBits::eCompute)
            add("Compute");
        if (f & vk::QueueFlagBits::eTransfer)
            add("Transfer");
        if (f & vk::QueueFlagBits::eSparseBinding)
            add("SparseBinding");
        if (f & vk::QueueFlagBits::eProtected)
            add("Protected");
        if (f & vk::QueueFlagBits::eVideoDecodeKHR)
            add("VideoDecodeKHR");
        if (f & vk::QueueFlagBits::eVideoEncodeKHR)
            add("VideoEncodeKHR");
        if (f & vk::QueueFlagBits::eOpticalFlowNV)
            add("OpticalFlowNV");
        if (first)
            oss << "None"; // if no flags recognized

        oss << " | timestampValidBits=" << q.timestampValidBits << " | minImageTransferGranularity=("
            << q.minImageTransferGranularity.width << ", " << q.minImageTransferGranularity.height << ", "
            << q.minImageTransferGranularity.depth << ")\n";
    }
    return oss.str();
}

inline std::string summarizeMemory(const vk::PhysicalDeviceMemoryProperties &mem)
{
    std::ostringstream oss;
    oss << "Memory Heaps (" << mem.memoryHeapCount << "):\n";
    for (uint32_t i = 0; i < mem.memoryHeapCount; ++i)
    {
        const auto &h = mem.memoryHeaps[i];
        oss << "  Heap[" << i << "]: size=" << formatBytes(h.size) << " flags=";
        bool first = true;
        auto add = [&](const char *s) {
            oss << (first ? "" : " | ") << s;
            first = false;
        };
        if (h.flags & vk::MemoryHeapFlagBits::eDeviceLocal)
            add("DeviceLocal");
        if (h.flags & vk::MemoryHeapFlagBits::eMultiInstanceKHR)
            add("MultiInstanceKHR");
        if (first)
            oss << "None";
        oss << "\n";
    }

    oss << "Memory Types (" << mem.memoryTypeCount << "):\n";
    for (uint32_t i = 0; i < mem.memoryTypeCount; ++i)
    {
        const auto &t = mem.memoryTypes[i];
        oss << "  Type[" << i << "]: heap=" << t.heapIndex << " | flags=";
        bool first = true;
        auto add = [&](const char *s) {
            oss << (first ? "" : ", ") << s;
            first = false;
        };
        auto flags = t.propertyFlags;
        if (flags & vk::MemoryPropertyFlagBits::eDeviceLocal)
            add("DeviceLocal");
        if (flags & vk::MemoryPropertyFlagBits::eHostVisible)
            add("HostVisible");
        if (flags & vk::MemoryPropertyFlagBits::eHostCoherent)
            add("HostCoherent");
        if (flags & vk::MemoryPropertyFlagBits::eHostCached)
            add("HostCached");
        if (flags & vk::MemoryPropertyFlagBits::eLazilyAllocated)
            add("LazilyAllocated");
        if (flags & vk::MemoryPropertyFlagBits::eProtected)
            add("Protected");
        if (flags & vk::MemoryPropertyFlagBits::eDeviceCoherentAMD)
            add("DeviceCoherentAMD");
        if (flags & vk::MemoryPropertyFlagBits::eDeviceUncachedAMD)
            add("DeviceUncachedAMD");
        if (first)
            oss << "None";
        oss << "\n";
    }
    return oss.str();
}

inline std::string summarizeExtensions(const std::vector<vk::ExtensionProperties> &exts)
{
    std::ostringstream oss;
    oss << "Device Extensions (" << exts.size() << "):\n";
    // Sort for stable output
    auto sorted = exts;
    std::sort(sorted.begin(), sorted.end(),
              [](auto &a, auto &b) { return std::string(a.extensionName.data()) < std::string(b.extensionName.data()); });
    for (const auto &e : sorted)
    {
        oss << "  " << e.extensionName << " (ver " << e.specVersion << ")\n";
    }
    return oss.str();
}

inline std::string summarizeFeatures(const vk::PhysicalDeviceFeatures &f)
{
    // Focus on commonly relevant features; you can expand as needed.
    std::ostringstream oss;
    oss << "Core Features (VkPhysicalDeviceFeatures):\n";
    auto onoff = [&](VkBool32 b) { return b ? "Yes" : "No"; };
    oss << "  geometryShader: " << onoff(f.geometryShader) << "\n"
        << "  tessellationShader: " << onoff(f.tessellationShader) << "\n"
        << "  multiDrawIndirect: " << onoff(f.multiDrawIndirect) << "\n"
        << "  drawIndirectFirstInstance: " << onoff(f.drawIndirectFirstInstance) << "\n"
        << "  shaderFloat64: " << onoff(f.shaderFloat64) << "\n"
        << "  samplerAnisotropy: " << onoff(f.samplerAnisotropy) << "\n"
        << "  textureCompressionBC: " << onoff(f.textureCompressionBC) << "\n"
        << "  fragmentStoresAndAtomics: " << onoff(f.fragmentStoresAndAtomics) << "\n"
        << "  shaderSampledImageArrayDynamicIndexing: " << onoff(f.shaderSampledImageArrayDynamicIndexing) << "\n"
        << "  shaderStorageImageArrayDynamicIndexing: " << onoff(f.shaderStorageImageArrayDynamicIndexing) << "\n"
        << "  dualSrcBlend: " << onoff(f.dualSrcBlend) << "\n"
        << "  wideLines: " << onoff(f.wideLines) << "\n";
    return oss.str();
}

/**
 * High-level summary string that aggregates properties, queues, memory, features, and extensions.
 * This is safe to call any time after instance/device enumeration.
 */
inline std::string makePhysicalDeviceLog(const vk::PhysicalDevice &pd)
{
    std::ostringstream oss;

    // Properties (core)
    vk::PhysicalDeviceProperties props = pd.getProperties();
    oss << summarizeProperties(props) << "\n";

    // Queue families
    auto queues = pd.getQueueFamilyProperties();
    oss << summarizeQueueFamilies(queues) << "\n";

    // Memory
    auto mem = pd.getMemoryProperties();
    oss << summarizeMemory(mem) << "\n";

    // Features (core)
    auto feats = pd.getFeatures();
    oss << summarizeFeatures(feats) << "\n";

    // Extensions
    auto exts = pd.enumerateDeviceExtensionProperties();
    oss << summarizeExtensions(exts) << "\n";

    // Optional: driver properties via pNext chain if available (VK_KHR_driver_properties)
    // This does not require enabling the extension to read properties.
    vk::PhysicalDeviceDriverProperties driverProps;
    vk::PhysicalDeviceProperties2 props2;
    props2.pNext = &driverProps;
    pd.getProperties2(&props2);
    oss << "Driver Properties (KHR):\n"
        << "  driverID: " << (uint32_t)driverProps.driverID << "\n"
        << "  driverName: " << driverProps.driverName << "\n"
        << "  driverInfo: " << driverProps.driverInfo << "\n";

    return oss.str();
}

} // namespace vklog
