#include "StarDevice.hpp"

namespace star
{
StarDevice::StarDevice(std::unique_ptr<Job::Manager> taskManager, StarWindow &window, std::set<star::Rendering_Features> requiredFeatures) : taskManager(std::move(taskManager)), starWindow(window)
{
    this->taskManager->startAll(); 

    if (requiredFeatures.find(star::Rendering_Features::shader_float64) != requiredFeatures.end())
        this->requiredDeviceFeatures.shaderFloat64 = VK_TRUE;

    this->requiredDeviceFeatures.geometryShader = VK_TRUE;
    this->requiredDeviceFeatures.samplerAnisotropy = VK_TRUE;
    this->requiredDeviceFeatures.fillModeNonSolid = VK_TRUE;
    this->requiredDeviceFeatures.logicOp = VK_TRUE;

    createInstance();

    this->starWindow.createWindowSurface(this->instance, this->surface);

    pickPhysicalDevice();
    createLogicalDevice();
    createAllocator();
}

std::unique_ptr<StarDevice> StarDevice::New(std::unique_ptr<Job::Manager> manager, StarWindow &window, std::set<star::Rendering_Features> requiredFeatures)
{
    return std::unique_ptr<StarDevice>(new StarDevice(std::move(manager), window, requiredFeatures));
}

StarDevice::~StarDevice()
{
    this->taskManager->stopAll(); 
    
    this->defaultCommandPool.reset();
    this->transferCommandPool.reset();
    this->computeCommandPool.reset();

    for (auto &buffer : this->extraFamilies)
    {
        buffer.reset();
    }

    this->allocator.reset();
    this->vulkanDevice.destroy();
    this->surface.reset();
    this->instance.destroy();
}

void StarDevice::createInstance()
{
    std::cout << "Creating vulkan instance..." << std::endl;

    if (enableValidationLayers && !CheckValidationLayerSupport(this->validationLayers))
    {
        throw std::runtime_error("validation layers requested, but not available");
    }

    vk::ApplicationInfo appInfo{};
    appInfo.sType = vk::StructureType::eApplicationInfo;
    appInfo.pApplicationName = "Starlight";
    appInfo.applicationVersion = vk::makeApiVersion(0, 1, 0, 0);
    appInfo.pEngineName = "Starlight";
    appInfo.engineVersion = vk::makeApiVersion(0, 1, 0, 0);
    appInfo.apiVersion = vk::ApiVersion13;

    // enumerate required extensions
    auto requiredInstanceExtensions = this->getRequiredExtensions();

    std::cout << "Instance requested with extensions: " << std::endl;
    for (auto ext : requiredInstanceExtensions)
    {
        std::cout << ext << std::endl;
    }

    {
        uint32_t extensionCount = uint32_t();
        CastHelpers::SafeCast<size_t, uint32_t>(requiredInstanceExtensions.size(), extensionCount);

        uint32_t validationLayerCount = uint32_t();
        CastHelpers::SafeCast<size_t, uint32_t>(validationLayers.size(), validationLayerCount);

        vk::InstanceCreateInfo createInfo =
            vk::InstanceCreateInfo()
                .setEnabledExtensionCount(extensionCount)
                .setPEnabledExtensionNames(requiredInstanceExtensions)
                .setEnabledLayerCount(this->enableValidationLayers ? validationLayerCount : 0)
                .setPEnabledLayerNames(this->enableValidationLayers ? validationLayers
                                                                    : vk::ArrayProxyNoTemporaries<const char *const>{})
                .setPApplicationInfo(&appInfo)
                .setFlags(this->isMac ? vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR
                                      : vk::InstanceCreateFlags());

        this->instance = vk::createInstance(createInfo);
    }

    /*
    All vulkan objects follow this pattern of creation :
    1.pointer to a struct with creation info
        2.pointer to custom allocator callbacks, (nullptr) here
        3.pointer to the variable that stores the handle to the new object
    */

    hasGlfwRequiredInstanceExtensions();
}

void StarDevice::pickPhysicalDevice()
{
    std::vector<vk::PhysicalDevice> devices = this->instance.enumeratePhysicalDevices();

    std::vector<vk::PhysicalDevice> suitableDevices;

    vk::PhysicalDevice picked;

    // check devices and see if they are suitable for use
    for (const auto &nDevice : devices)
    {
        if (IsDeviceSuitable(this->requiredDeviceExtensions, this->requiredDeviceFeatures, nDevice, *this->surface))
        {
            if (nDevice)
                suitableDevices.push_back(nDevice);
        }
    }

    // pick the best device of the potential devices that are suitable
    std::vector<vk::PhysicalDevice> optimalDevices = std::vector<vk::PhysicalDevice>();
    vk::PhysicalDevice optimalDevice;
    uint32_t largestQueueFamilyCount = 0;
    for (const auto &nDevice : suitableDevices)
    {
        auto indicies = FindQueueFamilies(nDevice, *this->surface);
        if (indicies.isOptimalSupport())
        {
            // try to pick the device that has the most seperate queue families
            if (indicies.getUniques().size() > largestQueueFamilyCount)
            {
                largestQueueFamilyCount = CastHelpers::size_t_to_unsigned_int(indicies.getUniques().size());
                optimalDevice = nDevice;
            }
        }
    }

    // check for a fully supported device
    if (!optimalDevice)
    {
        for (const auto &nDevice : devices)
        {
            auto indicies = FindQueueFamilies(nDevice, *this->surface);
            if (indicies.isFullySupported())
            {
                picked = nDevice;
            }
        }
    }
    else
    {
        picked = optimalDevice;
    }

    if (picked)
    {
        auto properties = picked.getProperties();
        std::cout << "Selected device properties:" << std::endl;
        if (optimalDevice)
        {
            std::cout << "Starlight Device Support: Optimal" << std::endl;
        }
        else
        {
            std::cout << "Starlight Device Support: Minimal" << std::endl;
        }
        std::cout << "Name: " << properties.deviceName << std::endl;
        std::cout << "Vulkan Api Version: " << properties.apiVersion << std::endl;
        this->physicalDevice = picked;
    }

    if ((devices.size() == 0) || !physicalDevice)
    {
        throw std::runtime_error("failed to find suitable GPU!");
    }
}

void StarDevice::createLogicalDevice()
{
    QueueFamilyIndicies indicies = FindQueueFamilies(this->physicalDevice, *this->surface);

    // need multiple structs since we now have a seperate family for presenting and graphics
    auto uniqueIndices = indicies.getUniques();
    std::set<uint32_t> uniqueQueueFamilies = indicies.getUniques();

    std::vector<StarQueueFamily> queueFamilies = std::vector<StarQueueFamily>();
    for (auto uniqueIndex : uniqueQueueFamilies) 
    {
        queueFamilies.emplace_back(uniqueIndex, indicies.getNumQueuesForIndex(uniqueIndex),
                                   indicies.getSupportForIndex(uniqueIndex),
                                   indicies.getSupportsPresentForIndex(uniqueIndex));
    }

    auto dynamicRenderingFeatures = vk::PhysicalDeviceDynamicRenderingFeatures().setDynamicRendering(true);

    auto syncFeatures =
        vk::PhysicalDeviceSynchronization2Features().setSynchronization2(true).setPNext(&dynamicRenderingFeatures);

    {
        uint32_t numDeviceExtensions = uint32_t();
        CastHelpers::SafeCast<size_t, uint32_t>(requiredDeviceExtensions.size(), numDeviceExtensions);

        uint32_t numValidationLayers = uint32_t();
        CastHelpers::SafeCast<size_t, uint32_t>(validationLayers.size(), numValidationLayers);

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        for (auto &family : queueFamilies){
            queueCreateInfos.push_back(family.getDeviceCreateInfo()); 
        }
        uint32_t numQueues = uint32_t();
        CastHelpers::SafeCast<size_t, uint32_t>(queueCreateInfos.size(), numQueues);

        const vk::DeviceCreateInfo createInfo = vk::DeviceCreateInfo()
                                                    .setQueueCreateInfoCount(numQueues)
                                                    .setPQueueCreateInfos(queueCreateInfos.data())
                                                    .setEnabledExtensionCount(numDeviceExtensions)
                                                    .setPEnabledExtensionNames(requiredDeviceExtensions)
                                                    .setPEnabledFeatures(&requiredDeviceFeatures)
                                                    .setEnabledLayerCount(numValidationLayers)
                                                    .setPEnabledLayerNames(validationLayers)
                                                    .setPNext(syncFeatures);

        // call to create the logical device
        this->vulkanDevice = physicalDevice.createDevice(createInfo);
    }

    for (auto &fam : queueFamilies){
        fam.init(this->vulkanDevice); 
    }
    this->currentDeviceQueues = std::make_unique<QueueOwnershipTracker>(queueFamilies);

    auto test = this->currentDeviceQueues->giveMeQueueWithProperties(
        vk::QueueFlagBits::eCompute & vk::QueueFlagBits::eTransfer & vk::QueueFlagBits::eGraphics, true); 
    this->defaultQueue = std::make_unique<StarQueue>(test.value());

    if (indicies.isFullySupported())
    {
        {
            const std::vector<uint32_t> indices = this->currentDeviceQueues->getQueueFamiliesWhichSupport(vk::QueueFlagBits::eCompute); 
            // pick queues from different families
            for (const uint32_t &index : indices)
            {
                if (this->defaultQueue->getParentQueueFamilyIndex() != index)
                {
                    std::unique_ptr<uint32_t> *target = nullptr;

                    auto fam =
                        this->currentDeviceQueues->giveMeQueueWithProperties(vk::QueueFlagBits::eCompute, false, index);
                    if (fam.has_value())
                    {
                        this->dedicatedComputeQueue = std::make_unique<StarQueue>(fam.value());
                        break;
                    }
                }
            }
        }

        {
            const std::vector<uint32_t> indices = this->currentDeviceQueues->getQueueFamiliesWhichSupport(vk::QueueFlagBits::eTransfer);
            for (const uint32_t &index : indices)
            {
                if (this->defaultQueue->getParentQueueFamilyIndex() != index && this->dedicatedComputeQueue->getParentQueueFamilyIndex() != index)
                {
                    std::unique_ptr<uint32_t> *target = nullptr;

                    auto fam =
                        this->currentDeviceQueues->giveMeQueueWithProperties(vk::QueueFlagBits::eTransfer, false, index);
                    if (fam.has_value())
                    {
                        this->dedicatedTransferQueue = std::make_unique<StarQueue>(fam.value());
                        break;
                    }
                }
            }
        }
    }

    this->defaultCommandPool =
        std::make_shared<StarCommandPool>(this->vulkanDevice, this->defaultQueue->getParentQueueFamilyIndex(), true);

    if (this->dedicatedComputeQueue != nullptr)
        this->computeCommandPool = std::make_shared<StarCommandPool>(this->vulkanDevice, this->dedicatedComputeQueue->getParentQueueFamilyIndex(), true); 

    if (this->dedicatedTransferQueue != nullptr)
        this->transferCommandPool = std::make_shared<StarCommandPool>(this->vulkanDevice, this->dedicatedTransferQueue->getParentQueueFamilyIndex(), true); 
}

void StarDevice::createAllocator()
{
    this->allocator =
        std::make_unique<star::Allocator>(this->getDevice(), this->getPhysicalDevice(), this->getInstance());
}

bool StarDevice::IsDeviceSuitable(const std::vector<const char *> &requiredDeviceExtensions,
                                  const vk::PhysicalDeviceFeatures &requiredDeviceFeatures,
                                  const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface)
{
    bool swapChainAdequate = false;
    QueueFamilyIndicies indicies = FindQueueFamilies(device, surface);
    bool extensionsSupported = CheckDeviceExtensionSupport(device, requiredDeviceExtensions);
    if (extensionsSupported && DoesDeviceSupportPresentation(device, surface))
    {
        SwapChainSupportDetails swapChainSupport = QuerySwapchainSupport(device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    vk::PhysicalDeviceFeatures supportedFeatures = device.getFeatures();
    bool supportsRequiredRenderingFeatures = true;

    if (requiredDeviceFeatures.samplerAnisotropy && !supportedFeatures.samplerAnisotropy)
        supportsRequiredRenderingFeatures = false;
    if (requiredDeviceFeatures.geometryShader && !supportedFeatures.geometryShader)
        supportsRequiredRenderingFeatures = false;

    if (indicies.isSuitable() && extensionsSupported && swapChainAdequate && supportsRequiredRenderingFeatures)
    {
        return true;
    }
    return false;
}

bool StarDevice::CheckValidationLayerSupport(const std::vector<const char *> &validationLayers)
{
    std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    for (const char *layerName : validationLayers)
    {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

std::vector<const char *> StarDevice::getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    // add required extensions for platform dependencies
    for (auto &extension : this->platformInstanceRequiredExtensions)
    {
        extensions.push_back(extension);
    }

    if (enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void StarDevice::hasGlfwRequiredInstanceExtensions()
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    std::unordered_set<std::string> available;
    for (const auto &extension : extensions)
    {
        available.insert(extension.extensionName);
    }

    // std::cout << "required extensions:" << std::endl;
    auto requiredExtensions = getRequiredExtensions();
    for (const auto &required : requiredExtensions)
    {
        if (available.find(required) == available.end())
        {
            throw std::runtime_error("Missing required glfw extension");
        }
    }
}

bool StarDevice::CheckDeviceExtensionSupport(const vk::PhysicalDevice &device,
                                             const std::vector<const char *> &requiredDeviceExtensions)
{
    uint32_t extensionCount;
    {
        auto result = device.enumerateDeviceExtensionProperties(nullptr, &extensionCount, nullptr);
        if (result != vk::Result::eSuccess)
            throw std::runtime_error("Failed to get number of device extension properties");
    }

    std::vector<vk::ExtensionProperties> availableExtensions(extensionCount);
    {
        auto result = device.enumerateDeviceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
        if (result != vk::Result::eSuccess)
            throw std::runtime_error("Failed to get all available device extensions");
    }

    std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());

    // iterate through extensions looking for those that are required
    for (const auto &extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

QueueFamilyIndicies StarDevice::FindQueueFamilies(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface)
{
    QueueFamilyIndicies indicies;

    std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

    uint32_t i = 0;
    for (const auto &queueFamily : queueFamilies)
    {
        vk::Bool32 presentSupport = device.getSurfaceSupportKHR(i, surface);

        indicies.registerFamily(i, queueFamily.queueFlags, presentSupport, queueFamily.queueCount);
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

    throw std::runtime_error("failed to find suitable memory type");
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

std::shared_ptr<star::StarCommandPool> StarDevice::getCommandPool(const star::Queue_Type &type)
{
    if (type == star::Queue_Type::Tcompute && this->computeCommandPool)
        return this->computeCommandPool;
    else if (type == star::Queue_Type::Ttransfer && this->transferCommandPool)
        return this->transferCommandPool;

    return this->defaultCommandPool;
}

StarQueue &StarDevice::getDefaultQueue(const star::Queue_Type &type)
{
    switch (type)
    {
    case (star::Queue_Type::Tpresent):
    case (star::Queue_Type::Tgraphics):
        return *this->defaultQueue;
        break;
        
    case (star::Queue_Type::Tcompute):
        if (this->dedicatedComputeQueue != nullptr)
        {
            return *this->dedicatedComputeQueue;
        }
        else
        {
            return *this->defaultQueue;
        }
        break;
    case (star::Queue_Type::Ttransfer):
        if (this->dedicatedTransferQueue != nullptr)
        {
            return *this->dedicatedTransferQueue;
        }
        else
        {
            return *this->defaultQueue;
        }
    default:
        throw std::runtime_error("Unable to find matching default queue with type");
    }
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
        throw std::runtime_error("failed to create buffer");
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
        throw std::runtime_error("failed to allocate buffer memory");
    }

    // 4th argument: offset within the region of memory. Since memory is allocated specifically for this vertex buffer,
    // the offset is 0 if not 0, required to be divisible by memRequirenments.alignment
    this->vulkanDevice.bindBufferMemory(buffer, bufferMemory, 0);
}

std::unique_ptr<star::StarCommandBuffer> StarDevice::beginSingleTimeCommands()
{
    assert(this->defaultCommandPool != nullptr);

    std::unique_ptr<StarCommandBuffer> tmpBuffer = std::make_unique<StarCommandBuffer>(
        this->getDevice(), 1, this->defaultCommandPool, Queue_Type::Tgraphics, true, false);

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit; // only planning on using this command buffer once

    tmpBuffer->begin(0, beginInfo);

    return tmpBuffer;
}

void StarDevice::endSingleTimeCommands(std::unique_ptr<StarCommandBuffer> commandBuff,
                                       vk::Semaphore *signalFinishSemaphore)
{
    commandBuff->buffer().end();

    commandBuff->submit(0, this->defaultQueue->getVulkanQueue());

    commandBuff->wait();
}

void StarDevice::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size,
                            const vk::DeviceSize dstOffset)
{
    std::unique_ptr<StarCommandBuffer> commandBuffer = beginSingleTimeCommands();

    vk::BufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size = size;

    // note: cannot specify VK_WHOLE_SIZE as before
    commandBuffer->buffer().copyBuffer(srcBuffer, dstBuffer, copyRegion);

    endSingleTimeCommands(std::move(commandBuffer));
}

void StarDevice::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height)
{
    std::unique_ptr<StarCommandBuffer> commandBuffer = beginSingleTimeCommands();

    // specify which region of the buffer will be copied to the image
    vk::BufferImageCopy region{};
    region.bufferOffset = 0; // specifies byte offset in the buffer at which the pixel values start
    // the following specify the layout of pixel information in memory
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    // the following indicate what part of the image we want to copy to
    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = vk::Offset3D{};
    region.imageExtent = vk::Extent3D{width, height, 1};

    // enque copy operation
    commandBuffer->buffer().copyBufferToImage(
        buffer, image,
        vk::ImageLayout::eTransferDstOptimal, // assuming image is already in optimal format for copy operations
        region);

    endSingleTimeCommands(std::move(commandBuffer));
}

void StarDevice::createImageWithInfo(const vk::ImageCreateInfo &imageInfo, vk::MemoryPropertyFlags properties,
                                     vk::Image &image, vk::DeviceMemory &imageMemory)
{

    image = this->vulkanDevice.createImage(imageInfo);
    if (!image)
    {
        throw std::runtime_error("failed to create image");
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
        throw std::runtime_error("failed to allocate image memory!");
    }

    this->vulkanDevice.bindImageMemory(image, imageMemory, 0);
}

SwapChainSupportDetails StarDevice::QuerySwapchainSupport(const vk::PhysicalDevice &device,
                                                          const vk::SurfaceKHR &surface)
{
    SwapChainSupportDetails details;
    uint32_t formatCount, presentModeCount;

    // get surface capabilities
    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);

    {
        const auto result = device.getSurfaceFormatsKHR(surface, &formatCount, nullptr);
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to get any surface formats from device");
        }
    }

    {
        const auto result = device.getSurfacePresentModesKHR(surface, &presentModeCount, nullptr);
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to get surface present modes from device");
        }
    }

    if (formatCount != 0)
    {
        // resize vector in order to hold all available formats
        details.formats.resize(formatCount);

        const auto result = device.getSurfaceFormatsKHR(surface, &formatCount, details.formats.data());
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to get surface present modes from device");
        }
    }

    if (presentModeCount != 0)
    {
        // resize for same reasons as format
        details.presentModes.resize(presentModeCount);

        const auto result = device.getSurfacePresentModesKHR(surface, &presentModeCount, details.presentModes.data());
        if (result != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to get surface present modes from device");
        }
    }

    return details;
}

bool StarDevice::DoesDeviceSupportPresentation(vk::PhysicalDevice physicalDevice, const vk::SurfaceKHR &surface)
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
} // namespace star