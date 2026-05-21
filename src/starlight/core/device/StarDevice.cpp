#include "device/StarDevice.hpp"

#include "starlight/core/Exceptions.hpp"
#include "starlight/core/logging/LoggingFactory.hpp"
#include "starlight/util/log/PhysicalDeviceLogging.hpp"

#include <star_common/helper/CastHelpers.hpp>

namespace star::core::device
{
inline static void LogPhysicalDeviceInfo(const vk::PhysicalDevice &physicalDevice)
{
    std::string log = "Selected physical device info: \n";
    log += star::log::makePhysicalDeviceLog(physicalDevice);
    star::core::logging::log(core::logging::LogLevel::info, log);
}

static vk::Device CreateLogicalDevice(vk::PhysicalDevice physicalDevice, core::RenderingInstance &instance,
                                      const vk::PhysicalDeviceFeatures &requiredDeviceFeatures,
                                      const std::vector<const char *> &requiredDeviceExtensions,
                                      const std::set<Rendering_Device_Features> &deviceFeatures,
                                      std::optional<vk::SurfaceKHR> renderingSurface)
{
    vk::Device device{VK_NULL_HANDLE};

    vk::PhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures =
        vk::PhysicalDeviceDynamicRenderingFeatures().setDynamicRendering(true);

    vk::PhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeature =
        vk::PhysicalDeviceTimelineSemaphoreFeatures().setTimelineSemaphore(true).setPNext(&dynamicRenderingFeatures);
    void *next = &dynamicRenderingFeatures;
    if (deviceFeatures.contains(Rendering_Device_Features::timeline_semaphores))
    {
        next = &timelineSemaphoreFeature;
    }
    auto syncFeatures = vk::PhysicalDeviceSynchronization2Features().setSynchronization2(true).setPNext(next);

    {
        auto queueInfo = StarDevice::FindQueueFamilies(
            physicalDevice, renderingSurface.has_value() ? &renderingSurface.value() : nullptr);
        auto families = queueInfo.getQueueFamilies();

        uint32_t numDeviceExtensions = uint32_t();
        star::common::casts::SafeCast<size_t, uint32_t>(requiredDeviceExtensions.size(), numDeviceExtensions);

        std::vector<const char *> validationLayerNames = instance.getValidationLayerNames();
        uint32_t numValidationLayers = uint32_t();
        star::common::casts::SafeCast<size_t, uint32_t>(validationLayerNames.size(), numValidationLayers);

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos =
            std::vector<vk::DeviceQueueCreateInfo>(families.size());
        for (size_t i{0}; i < queueCreateInfos.size(); i++)
        {
            queueCreateInfos[i] = vk::DeviceQueueCreateInfo()
                                      .setQueueFamilyIndex(families[i].getQueueFamilyIndex())
                                      .setQueueCount(families[i].getQueueCount())
                                      .setPQueuePriorities(families[i].getQueuePriorities().data());
        }

        uint32_t numQueues = 0;
        star::common::casts::SafeCast(queueCreateInfos.size(), numQueues);

        const vk::DeviceCreateInfo createInfo = vk::DeviceCreateInfo()
                                                    .setQueueCreateInfoCount(numQueues)
                                                    .setPQueueCreateInfos(queueCreateInfos.data())
                                                    .setEnabledExtensionCount(numDeviceExtensions)
                                                    .setPEnabledExtensionNames(requiredDeviceExtensions)
                                                    .setPEnabledFeatures(&requiredDeviceFeatures)
                                                    .setEnabledLayerCount(numValidationLayers)
                                                    .setPEnabledLayerNames(validationLayerNames)
                                                    .setPNext(syncFeatures);

        // call to create the logical device
        device = physicalDevice.createDevice(createInfo);
    }

    return device;
}

static bool DoesDeviceSupportPresentation(vk::PhysicalDevice physicalDevice, const vk::SurfaceKHR &surface)
{
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        if (physicalDevice.getSurfaceSupportKHR(i, surface))
            return true;
    }
    return false;
}

static bool CheckDeviceExtensionSupport(const vk::PhysicalDevice &device,
                                        const std::vector<const char *> &requiredDeviceExtensions)
{
    uint32_t extensionCount;
    {
        auto result = device.enumerateDeviceExtensionProperties(nullptr, &extensionCount, nullptr);
        if (result != vk::Result::eSuccess)
        {
            STAR_THROW("Failed to get number of device extension properties");
        }
    }

    std::vector<vk::ExtensionProperties> availableExtensions(extensionCount);
    {
        auto result = device.enumerateDeviceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
        if (result != vk::Result::eSuccess)
        {
            STAR_THROW("Failed to get all available device extensions");
        }
    }

    std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());

    // iterate through extensions looking for those that are required
    for (const auto &extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

static bool IsDeviceSuitable(const std::vector<const char *> &requiredDeviceExtensions,
                             const vk::PhysicalDeviceFeatures &requiredDeviceFeatures, const vk::PhysicalDevice &device,
                             const vk::SurfaceKHR *optionalRenderingSurface)
{
    bool swapChainAdequate = false;
    QueueFamilyIndices indicies = StarDevice::FindQueueFamilies(device, optionalRenderingSurface);
    bool extensionsSupported = CheckDeviceExtensionSupport(device, requiredDeviceExtensions);
    if (extensionsSupported && optionalRenderingSurface != nullptr &&
        DoesDeviceSupportPresentation(device, *optionalRenderingSurface))
    {
        core::SwapChainSupportDetails swapChainSupport =
            StarDevice::QuerySwapchainSupport(device, *optionalRenderingSurface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    vk::PhysicalDeviceFeatures supportedFeatures = device.getFeatures();
    bool supportsRequiredRenderingFeatures = true;

    if (requiredDeviceFeatures.samplerAnisotropy && !supportedFeatures.samplerAnisotropy)
        supportsRequiredRenderingFeatures = false;
    if (requiredDeviceFeatures.geometryShader && !supportedFeatures.geometryShader)
        supportsRequiredRenderingFeatures = false;

    const bool properQueueFamilySupport = indicies.isSuitable(optionalRenderingSurface ? true : false);
    if (properQueueFamilySupport && extensionsSupported && supportsRequiredRenderingFeatures &&
        (!optionalRenderingSurface || (optionalRenderingSurface && swapChainAdequate)))
    {
        return true;
    }
    return false;
}

vk::PhysicalDevice StarDevice::Builder::pickPhysicalDevice(vk::PhysicalDeviceFeatures deviceFeatures) const
{
    const bool needsPresentationSupport = m_surface ? true : false;
    std::vector<vk::PhysicalDevice> devices = m_instance.getVulkanInstance().enumeratePhysicalDevices();
    std::vector<vk::PhysicalDevice> suitableDevices;
    suitableDevices.reserve(devices.size());
    vk::PhysicalDevice picked{VK_NULL_HANDLE};

    // check devices and see if they are suitable for use
    if (m_overrideDevice.has_value() && m_overrideDevice.value().deviceID != -1)
    {
        if (m_overrideDevice.value().deviceID > devices.size())
        {
            std::ostringstream oss;
            oss << "Attempted to manually select device by index which is larger than available devices. Selected "
                   "index: "
                << m_overrideDevice.value().deviceID;
            throw std::invalid_argument(oss.str());
        }
        if (IsDeviceSuitable(m_extensions, deviceFeatures, devices[m_overrideDevice.value().deviceID],
                             m_surface.has_value() ? &m_surface.value() : nullptr))
        {
            suitableDevices.push_back(devices[m_overrideDevice.value().deviceID]);
        }
    }
    else
    {
        for (const auto &nDevice : devices)
        {
            if (IsDeviceSuitable(m_extensions, deviceFeatures, nDevice,
                                 m_surface.has_value() ? &m_surface.value() : nullptr))
            {
                if (nDevice)
                    suitableDevices.push_back(nDevice);
            }
        }
    }

    // pick the best device of the potential devices that are suitable
    std::vector<vk::PhysicalDevice> optimalDevices = std::vector<vk::PhysicalDevice>();
    uint32_t largestQueueFamilyCount = 0;
    for (const auto &nDevice : suitableDevices)
    {
        auto indicies = FindQueueFamilies(nDevice, m_surface.has_value() ? &m_surface.value() : nullptr);
        if (indicies.isOptimalSupport(needsPresentationSupport))
        {
            // try to pick the device that has the most seperate queue families
            if (indicies.getUniques().size() > largestQueueFamilyCount)
            {
                largestQueueFamilyCount = star::common::casts::size_t_to_unsigned_int(indicies.getUniques().size());
                picked = nDevice;
            }
        }
    }

    // check for a fully supported device
    if (!picked)
    {
        for (const auto &nDevice : devices)
        {
            auto indicies = FindQueueFamilies(nDevice, m_surface.has_value() ? &m_surface.value() : nullptr);
            if (indicies.isFullySupported(needsPresentationSupport))
            {
                picked = nDevice;
            }
        }
    }

    return picked;
}

StarDevice::Builder &StarDevice::Builder::setOverrideDeviceID(int deviceID)
{
    assert(!m_overrideDevice);
    m_overrideDevice = OverridenDeviceSelected{.deviceID = std::move(deviceID)};
    return *this;
}

StarDevice::Builder &StarDevice::Builder::setRenderingFeatures(std::set<star::Rendering_Features> renderingFeatures)
{
    m_deviceRenderingFeatures = std::move(renderingFeatures);
    return *this;
}

StarDevice::Builder &StarDevice::Builder::setRenderingDeviceFeatures(std::set<Rendering_Device_Features> features)
{
    m_deviceFeatures = std::move(features);
    return *this;
}

StarDevice::Builder &StarDevice::Builder::setAdditionalExtensions(const std::vector<const char *> &extensions)
{
    m_extensions = extensions;
    return *this;
}

StarDevice::Builder &StarDevice::Builder::setOptionalSurface(vk::SurfaceKHR surface)
{
    m_surface = surface;
    return *this;
}

StarDevice StarDevice::Builder::build()
{
    vk::PhysicalDevice physicalDevice{VK_NULL_HANDLE};
    vk::Device device{VK_NULL_HANDLE};

    vk::PhysicalDeviceFeatures requiredPhysicalDeviceFeatures{};
    requiredPhysicalDeviceFeatures.geometryShader = VK_TRUE;
    requiredPhysicalDeviceFeatures.samplerAnisotropy = VK_TRUE;
    requiredPhysicalDeviceFeatures.fillModeNonSolid = VK_TRUE;
    requiredPhysicalDeviceFeatures.logicOp = VK_TRUE;
    if (m_deviceRenderingFeatures.find(star::Rendering_Features::shader_float64) != m_deviceRenderingFeatures.end())
        requiredPhysicalDeviceFeatures.shaderFloat64 = VK_TRUE;

    auto physicalDeviceExtensions = GetRequiredDeviceExtensions();
    for (size_t i{0}; i < m_extensions.size(); i++)
    {
        physicalDeviceExtensions.emplace_back(m_extensions[i]);
    }

    physicalDevice = pickPhysicalDevice(requiredPhysicalDeviceFeatures);
    LogPhysicalDeviceInfo(physicalDevice);
    if (physicalDevice == VK_NULL_HANDLE)
    {
        STAR_THROW("Failed to pick a proper physical device");
    }

    device = CreateLogicalDevice(physicalDevice, m_instance, requiredPhysicalDeviceFeatures, m_extensions,
                                 m_deviceFeatures, m_surface);
    if (device == VK_NULL_HANDLE)
        STAR_THROW("Failed to create logical vulkan device");

    Allocator allocator(device, physicalDevice, m_instance.getVulkanInstance());
    return m_surface ? StarDevice(std::move(allocator), std::move(device), std::move(physicalDevice), m_surface.value())
                     : StarDevice(std::move(allocator), std::move(device), std::move(physicalDevice));
}

StarDevice::StarDevice(StarDevice &&other) noexcept
    : vulkanDevice(other.vulkanDevice), allocator(std::move(other.allocator)), physicalDevice(other.physicalDevice)
{
    other.vulkanDevice = VK_NULL_HANDLE;
}

StarDevice &StarDevice::operator=(StarDevice &&other) noexcept
{
    if (this != &other)
    {
        vulkanDevice = std::move(other.vulkanDevice);
        allocator = std::move(other.allocator);
        physicalDevice = std::move(other.physicalDevice);

        other.vulkanDevice = VK_NULL_HANDLE;
    }

    return *this;
}

StarDevice::StarDevice(star::Allocator allocator, vk::Device device, vk::PhysicalDevice physicalDevice)
    : allocator(std::move(allocator)), vulkanDevice(std::move(device)), physicalDevice(std::move(physicalDevice))
{
}

StarDevice::StarDevice(star::Allocator allocator, vk::Device device, vk::PhysicalDevice physicalDevice,
                       vk::SurfaceKHR optionalRenderingSurface)
    : allocator(std::move(allocator)), vulkanDevice(std::move(device)), physicalDevice(std::move(physicalDevice)),
      m_renderingSurface(std::move(optionalRenderingSurface))
{
}

StarDevice::~StarDevice()
{
    if (vulkanDevice != VK_NULL_HANDLE)
    {
        allocator.cleanupRender();
        vulkanDevice.destroy();
        vulkanDevice = VK_NULL_HANDLE;
    }
}

QueueFamilyIndices StarDevice::getQueueInfo()
{
    return FindQueueFamilies(physicalDevice, m_renderingSurface.has_value() ? &m_renderingSurface.value() : nullptr);
}

QueueFamilyIndices StarDevice::FindQueueFamilies(const vk::PhysicalDevice &device,
                                                 const vk::SurfaceKHR *optionalRenderingSurface)
{
    QueueFamilyIndices indicies;

    std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

    uint32_t i = 0;
    for (const auto &queueFamily : queueFamilies)
    {
        bool supportPresent = false;
        if (optionalRenderingSurface != nullptr && device.getSurfaceSupportKHR(i, *optionalRenderingSurface))
        {
            supportPresent = true;
        }

        indicies.registerFamily(i, queueFamily.queueFlags, supportPresent, queueFamily.queueCount);
        i++;
    }

    return indicies;
}

uint32_t StarDevice::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags propertyFlags)
{
    // query available memory -- right now only concerned with memory type, not the heap that it comes from
    /*VkPhysicalDeviceMemoryProperties contains:
    1. memoryTypes
    2. memoryHeaps - distinct memory resources (dedicated VRAM or swap space)
    */
    vk::PhysicalDeviceMemoryProperties memProperties = this->physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        // use binary AND to test each bit (Left Shift)
        // check memory types array for more detailed information on memory capabilities
        // we need to be able to write to memory, so speficially looking to be able to MAP to the memory to write to it
        // from the CPU -- VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        // also need VK_MEMORY_PROPERTY_HOST_COHERENT_BIT

        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
        {
            return i;
        }
    }

    STAR_THROW("failed to find suitable memory type");
}

bool StarDevice::verifyImageCreate(vk::ImageCreateInfo imageInfo)
{
    try
    {
        vk::ImageFormatProperties pros = this->physicalDevice.getImageFormatProperties(
            imageInfo.format, imageInfo.imageType, imageInfo.tiling, imageInfo.usage, imageInfo.flags);
    }
    catch (const std::exception &ex)
    {
        std::cout << "An error occurred while attempting to verify new image: " << ex.what() << std::endl;
        return false;
    }
    return true;
}

bool StarDevice::findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling,
                                     vk::FormatFeatureFlags features, vk::Format &selectedFormat) const
{
    for (vk::Format format : candidates)
    {
        // VkFormatProperties:
        // linearTilingFeatures
        // optimalTilingFeatures
        // bufferFeatures
        vk::FormatProperties props = this->physicalDevice.getFormatProperties(format);

        // check if the properties matches the requirenments for tiling
        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
        {
            selectedFormat = format;
            return true;
        }
        else if ((tiling == vk::ImageTiling::eOptimal) && (props.optimalTilingFeatures & features) == features)
        {
            selectedFormat = format;
            return true;
        }
    }

    return false;
}

void StarDevice::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usageFlags, vk::MemoryPropertyFlags properties,
                              vk::Buffer &buffer, vk::DeviceMemory &bufferMemory)
{
    vk::BufferCreateInfo bufferInfo{};

    bufferInfo.sType = vk::StructureType::eBufferCreateInfo;
    bufferInfo.size = size;
    bufferInfo.usage = usageFlags; // purpose of data in buffer
    bufferInfo.sharingMode =
        vk::SharingMode::eExclusive; // buffers can be owned by specific queue family or shared between them at the same
                                     // time. This only used for graphics queue

    buffer = this->vulkanDevice.createBuffer(bufferInfo);

    if (!buffer)
    {
        STAR_THROW("Failed to create buffer");
    }

    // need to allocate memory for the buffer object
    /* VkMemoryRequirements:
        1. size - number of required bytes in memory
        2. alignments - offset in bytes where the buffer begins in the allocated region of memory (depends on
       bufferInfo.useage and bufferInfo.flags)
        3. memoryTypeBits - bit fied of the memory types that are suitable for the buffer
    */
    vk::MemoryRequirements memRequirenments = this->vulkanDevice.getBufferMemoryRequirements(buffer);

    assert(size <= memRequirenments.size);

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eMemoryAllocateInfo;
    allocInfo.allocationSize = memRequirenments.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirenments.memoryTypeBits, properties);

    bufferMemory = this->vulkanDevice.allocateMemory(allocInfo);

    // should not call vkAllocateMemory for every object. Bundle objects into one call and use offsets
    if (!bufferMemory)
    {
        STAR_THROW("failed to allocate buffer memory");
    }

    // 4th argument: offset within the region of memory. Since memory is allocated specifically for this vertex buffer,
    // the offset is 0 if not 0, required to be divisible by memRequirenments.alignment
    this->vulkanDevice.bindBufferMemory(buffer, bufferMemory, 0);
}

void StarDevice::createImageWithInfo(const vk::ImageCreateInfo &imageInfo, vk::MemoryPropertyFlags properties,
                                     vk::Image &image, vk::DeviceMemory &imageMemory)
{

    image = this->vulkanDevice.createImage(imageInfo);
    if (!image)
    {
        STAR_THROW("Failed to create vulkan image");
    }

    /* Allocate the memory for the imag*/
    vk::MemoryRequirements memRequirements = this->vulkanDevice.getImageMemoryRequirements(image);

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.sType = vk::StructureType::eMemoryAllocateInfo;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    imageMemory = this->vulkanDevice.allocateMemory(allocInfo);
    if (!imageMemory)
    {
        STAR_THROW("Failed to allocate image memory");
    }

    this->vulkanDevice.bindImageMemory(image, imageMemory, 0);
}

core::SwapChainSupportDetails StarDevice::QuerySwapchainSupport(const vk::PhysicalDevice &device,
                                                                vk::SurfaceKHR surface)
{
    core::SwapChainSupportDetails details;
    uint32_t formatCount, presentModeCount;

    // get surface capabilities
    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);

    {
        const auto result = device.getSurfaceFormatsKHR(surface, &formatCount, nullptr);
        if (result != vk::Result::eSuccess)
        {
            STAR_THROW("Failed to get any surface formats from device");
        }
    }

    {
        const auto result = device.getSurfacePresentModesKHR(surface, &presentModeCount, nullptr);
        if (result != vk::Result::eSuccess)
        {
            STAR_THROW("Failed to get surface present modes from device");
        }
    }

    if (formatCount != 0)
    {
        // resize vector in order to hold all available formats
        details.formats.resize(formatCount);

        const auto result = device.getSurfaceFormatsKHR(surface, &formatCount, details.formats.data());
        if (result != vk::Result::eSuccess)
        {
            STAR_THROW("Failed to get surface present modes from device");
        }
    }

    if (presentModeCount != 0)
    {
        // resize for same reasons as format
        details.presentModes.resize(presentModeCount);

        const auto result = device.getSurfacePresentModesKHR(surface, &presentModeCount, details.presentModes.data());
        if (result != vk::Result::eSuccess)
        {
            STAR_THROW("Failed to get surface present modes from device");
        }
    }

    return details;
}
} // namespace star::core::device