#include "TransferWorker.hpp"

// std::unique_ptr<star::StarBuffer> star::TransferManagerThread::syncCreate(star::BufferManagerRequest* newBufferRequest, boost::atomic<vk::Fence>& workCompleteFence){
//     assert(!this->thread.joinable() && "This function should only be called in single threaded mode. The instance of this transferThread has a child thread running");

//     return createBuffer(this->myDevice, this->myTransferQueue, this->myAllocator, newBufferRequest, workCompleteFence);
// }

// std::unique_ptr<star::StarBuffer> star::TransferThread::createBuffer(vk::Device& device, vk::Queue& transferQueue, Allocator& allocator, BufferManagerRequest *newBufferRequest, boost::atomic<vk::Fence>& workCompleteFence){

//     //create resulting buffer
//     auto stagingBuffer = std::make_unique<StarBuffer>(
//         allocator, 
//         newBufferRequest->getCreationArgs().instanceSize, 
//         newBufferRequest->getCreationArgs().instanceCount,
//         (VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT),
//         (VMA_MEMORY_USAGE_AUTO),
//         vk::BufferUsageFlagBits::eTransferSrc,
//         newBufferRequest->getCreationArgs().sharingMode,
//         1
//     );

//     //create resulting buffer
//     newBufferRequest->write(*stagingBuffer);
    
//     //submit copy commands


//     return std::unique_ptr<StarBuffer>();
// }

std::vector<vk::CommandBuffer> star::TransferManagerThread::createCommandBuffers(vk::Device& device, vk::CommandPool pool, const uint8_t& numToCreate){
    vk::CommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandPool = pool;
	allocInfo.commandBufferCount = numToCreate;

	return device.allocateCommandBuffers(allocInfo);
}

std::vector<vk::Queue> star::TransferManagerThread::createTransferQueues(star::StarDevice& device, const uint32_t& dedicatedTransferQueueFamilyIndex){
    std::vector<vk::Queue> queues = std::vector<vk::Queue>(); 

    std::vector<vk::QueueFamilyProperties> queueFamilies = device.getPhysicalDevice().getQueueFamilyProperties();
    if (dedicatedTransferQueueFamilyIndex > queueFamilies.size()){
        throw std::runtime_error("Invalid Queue Family Index");
    }

    uint32_t& numQueues = queueFamilies[dedicatedTransferQueueFamilyIndex].queueCount;
    for(uint32_t i = 0; i < numQueues; ++i){
        queues.push_back(device.getDevice().getQueue(dedicatedTransferQueueFamilyIndex, i));
    }

    return queues;
}

std::unique_ptr<star::StarBuffer> star::TransferManagerThread::createBuffer(vk::Device& device, Allocator& allocator, vk::Queue& transferQueue, vk::PhysicalDeviceLimits& limits, vk::Fence* workCompleteFence, BufferManagerRequest *newBufferRequest, std::queue<std::unique_ptr<star::TransferManagerThread::InProcessRequestDependencies>>& inProcessRequests, size_t bufferIndexToUse, std::vector<vk::CommandBuffer>& commandBuffers, std::vector<vk::Fence*>& commandBufferFences) {
    assert(commandBufferFences[bufferIndexToUse] == nullptr && "Command buffer fence should have already been waited on and removed");
    
    auto transferSrcBuffer = std::make_unique<StarBuffer>(
        allocator, 
        newBufferRequest->getCreationArgs().instanceSize,
        newBufferRequest->getCreationArgs().instanceCount,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::SharingMode::eConcurrent,
        (newBufferRequest->getCreationArgs().useFlags & vk::BufferUsageFlagBits::eUniformBuffer) ? limits.minUniformBufferOffsetAlignment : 1
    );

    auto destinationBuffer = std::make_unique<StarBuffer>(
        allocator, 
        newBufferRequest->getCreationArgs().instanceSize,
        newBufferRequest->getCreationArgs().instanceCount,
        newBufferRequest->getCreationArgs().creationFlags,
        newBufferRequest->getCreationArgs().memoryUsageFlags,
        newBufferRequest->getCreationArgs().useFlags | vk::BufferUsageFlagBits::eTransferDst,
        vk::SharingMode::eConcurrent, 
        (newBufferRequest->getCreationArgs().useFlags & vk::BufferUsageFlagBits::eUniformBuffer) ? limits.minUniformBufferOffsetAlignment : 1
    );

    newBufferRequest->write(*transferSrcBuffer);

    //copy operations
    vk::CommandBuffer& commandBuffer = commandBuffers[bufferIndexToUse];
    {
        vk::CommandBufferBeginInfo beginInfo{};
        beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    
        commandBuffer.begin(beginInfo);
    }

    {
        vk::BufferCopy copyRegion{}; 
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = transferSrcBuffer->getBufferSize(); 
    
        commandBuffer.copyBuffer(transferSrcBuffer->getVulkanBuffer(), destinationBuffer->getVulkanBuffer(), copyRegion);
    }

    commandBuffer.end();

    {
        vk::SubmitInfo submitInfo{};
        submitInfo.pCommandBuffers = &commandBuffer; 
        submitInfo.commandBufferCount = 1;

        auto commandResult = std::make_unique<vk::Result>(transferQueue.submit(1, &submitInfo, *workCompleteFence)); 

        if (*commandResult != vk::Result::eSuccess){
            //handle error
            std::runtime_error("Failed to submit transfer request"); 
        }
    }

    inProcessRequests.push(std::make_unique<InProcessRequestDependencies>(std::move(transferSrcBuffer), workCompleteFence));
    commandBufferFences[bufferIndexToUse] = workCompleteFence; 

    // inProcessRequests.push_back(std::make_unique<InProcessRequestDependencies>(std::move(transferSrcBuffer), workCompleteFence));
    return destinationBuffer; 
}

void star::TransferManagerThread::checkForCleanups(vk::Device& device, std::queue<std::unique_ptr<star::TransferManagerThread::InProcessRequestDependencies>>& inProcessRequests, std::vector<vk::Fence*>& commandBufferFences){
    
    std::queue<std::unique_ptr<InProcessRequestDependencies>> stillInProcess = std::queue<std::unique_ptr<InProcessRequestDependencies>>();

    while(!inProcessRequests.empty()){
        std::unique_ptr<InProcessRequestDependencies>& deps = inProcessRequests.back();

        auto fenceResult = device.getFenceStatus(*deps->completeFence);
        if(fenceResult != vk::Result::eSuccess){
            stillInProcess.push(std::move(deps));
        }

        inProcessRequests.pop();
    }

    inProcessRequests = std::move(stillInProcess);
}

void star::TransferManagerThread::readyCommandBuffer(vk::Device& device, const size_t& indexSelected, std::vector<vk::Fence*>& commandBufferFences){
    //sometimes multiple updates can be called on a buffer which shares a fence with the commandBuffers, if this happens, the manager is expected to reset the fence 
    if (commandBufferFences[indexSelected] != nullptr){
        auto result = device.waitForFences(*commandBufferFences[indexSelected], true, UINT64_MAX);

        if (result != vk::Result::eSuccess)
            throw std::runtime_error("Failed to wait for fence"); 

        commandBufferFences[indexSelected] = nullptr; 
    }
}


star::TransferManagerThread::TransferManagerThread(vk::Device& device, star::Allocator& allocator, vk::PhysicalDeviceLimits deviceLimits)
: device(device), deviceLimits(deviceLimits), ownsVulkanResources(true), allocator(allocator) {
    // device.createPool(myTransferPoolIndex, vk::CommandPoolCreateFlagBits::eResetCommandBuffer, this->transferPool);
    // this->transferQueues = createTransferQueues(device, myTransferPoolIndex);
    // this->commandBuffers = createCommandBuffers(device, this->transferPool, 1);
}

star::TransferManagerThread::TransferManagerThread(vk::Device& device, star::Allocator& allocator, vk::PhysicalDeviceLimits deviceLimits, vk::Queue sharedTransferQueue, vk::CommandPool sharedCommandPool)
: device(device), allocator(allocator), transferPool(sharedCommandPool), transferQueues({sharedTransferQueue}), 
commandBuffers(createCommandBuffers(device, sharedCommandPool, 5)), commandBufferFences(std::vector<vk::Fence*>(5)), deviceLimits(deviceLimits){

}

void star::TransferManagerThread::add(star::TransferManagerThread::InterThreadRequest request, const bool& isHighPriority){
    assert(this->device.getFenceStatus(*request.completeFence) == vk::Result::eSuccess && "Fences MUST be submitting in a signaled state to the worker");
    
    if (this->thread.joinable()){
        if (isHighPriority){
            this->highPriorityRequests.value().push(request);
        }else{
            this->standardTransferRequests.value().push(request);
        }
    }else{
        size_t targetBufferIndex = 0;
        if (this->previousBufferIndexUsed != this->commandBuffers.size()-1){
            targetBufferIndex = this->previousBufferIndexUsed + 1;
            this->previousBufferIndexUsed++; 
        }

        readyCommandBuffer(this->device, targetBufferIndex, this->commandBufferFences); 

        this->device.resetFences(*request.completeFence); 

        *request.resultingBuffer.value() = createBuffer(this->device, this->allocator, 
            this->transferQueues.back(), this->deviceLimits, request.completeFence, request.bufferTransferRequest.value(), this->inProcessRequests, targetBufferIndex, this->commandBuffers, this->commandBufferFences);

        this->previousBufferIndexUsed = targetBufferIndex; 
    }
}

void star::TransferManagerThread::inSyncCleanup(){
    checkForCleanups(this->device, this->inProcessRequests, this->commandBufferFences);
}

star::TransferWorker::~TransferWorker() {
    if (this->asyncMode){
        // for (auto& worker : this->workers){
        //     worker->stop();
        // }
    }
}

star::TransferWorker::TransferWorker(star::StarDevice& device, star::Allocator& allocator) 
: asyncMode(true), manager(std::make_unique<TransferManagerThread>(device.getDevice(), allocator, device.getPhysicalDevice().getProperties().limits)) {

}

star::TransferWorker::TransferWorker(star::StarDevice& device, star::Allocator& allocator, vk::Queue sharedTransferQueue, vk::CommandPool sharedCommandPool)
: asyncMode(false), manager(std::make_unique<TransferManagerThread>(device.getDevice(), allocator, device.getPhysicalDevice().getProperties().limits, sharedTransferQueue, sharedCommandPool)) {

}

void star::TransferWorker::add(star::BufferManagerRequest& newBufferRequest, vk::Fence* workCompleteFence, std::unique_ptr<star::StarBuffer>& resultingBuffer, const bool& isHighPriority){
    auto request = TransferManagerThread::InterThreadRequest(newBufferRequest, resultingBuffer, workCompleteFence);
    
    this->manager->add(request, isHighPriority);
}

star::TransferManagerThread::~TransferManagerThread(){
    if (this->ownsVulkanResources){
        //multithreaded mode
        //means that this class is responsible for destroying the transfer pool it was using

    }else{
        //single threaded mode

    }

    //make sure to wait for all child threads to exit

    //destroy transferpool

    //sync mode
}


void star::TransferWorker::update(){
    if (!this->asyncMode){
        this->manager->inSyncCleanup();
    }
}