#include "TransferWorker.hpp"

star::TransferManagerThread::~TransferManagerThread(){
    if (this->thread.joinable()){
        this->stopAsync();

        this->thread.join();
    }
}


void star::TransferManagerThread::initMultithreadedDeps(){
    this->highPriorityRequests.emplace(50);
    this->standardRequests.emplace(50);
}

void star::TransferManagerThread::startAsync(){
   this->initMultithreadedDeps();

    this->shouldRun.store(true);
    this->thread = boost::thread(TransferManagerThread::mainLoop, 
        &this->shouldRun, 
        &this->device.getDevice(),
        &this->transferQueue->getCommandPool(),
        &this->transferQueue->getQueue(), 
        &this->allocator.get(), 
        &this->deviceLimits,
        &this->commandBufferFences,
        &this->highPriorityRequests.value(), 
        &this->standardRequests.value()
    );

    if (!this->thread.joinable())
        throw std::runtime_error("Failed to launch transfer thread");
}

void star::TransferManagerThread::stopAsync(){
    this->shouldRun.store(false);
            
    //wait for thread to exit
    this->thread.join();
}

void star::TransferManagerThread::mainLoop(boost::atomic<bool>* shouldRun, vk::Device* device, vk::CommandPool* transferPool, vk::Queue* transferQueue, VmaAllocator* allocator, vk::PhysicalDeviceLimits* limits, std::vector<SharedFence*>* commandBufferFences, boost::lockfree::stack<star::TransferManagerThread::InterThreadRequest*>* highPriorityRequests, boost::lockfree::stack<star::TransferManagerThread::InterThreadRequest*>* standardRequests){
    std::cout << "Transfer thread started..." << std::endl;
    size_t targetBufferIndex = 0;
    size_t previousBufferIndexUsed = 0;
    std::vector<vk::CommandBuffer> commandBuffers = createCommandBuffers(*device, *transferPool, 5);
    std::queue<std::unique_ptr<TransferManagerThread::InProcessRequestDependencies>> inProcessRequests = std::queue<std::unique_ptr<TransferManagerThread::InProcessRequestDependencies>>();

    while(shouldRun->load()){
        InterThreadRequest* request = nullptr; 
        if(!highPriorityRequests->pop(request) && !standardRequests->pop(request)){
            if (!inProcessRequests.empty()){
                checkForCleanups(*device, inProcessRequests, *commandBufferFences);
            }

            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }else{
            if (!request->bufferTransferRequest){
                std::runtime_error("Does not support textures yet.");
            }else{

                if (previousBufferIndexUsed != commandBuffers.size()-1){
                    targetBufferIndex = previousBufferIndexUsed + 1;
                    previousBufferIndexUsed++; 
                }
        
                readyCommandBuffer(*device, targetBufferIndex, *commandBufferFences); 
                
                createBuffer(*device, 
                    *allocator, 
                    *transferQueue, 
                    *limits, 
                    *request->completeFence, 
                    request->bufferTransferRequest.get(), 
                    inProcessRequests, 
                    targetBufferIndex, 
                    commandBuffers, 
                    *commandBufferFences, 
                    request->resultingBuffer.value());                
            }

            request->bufferTransferRequest.get()->afterWriteData();

            request->cpuWorkDoneByTransferThread->store(true);
            request->cpuWorkDoneByTransferThread->notify_one();
        }
    }

    std::cout << "Transfer Thread exiting..." << std::endl;
}

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

void star::TransferManagerThread::createBuffer(vk::Device& device, VmaAllocator& allocator, vk::Queue& transferQueue, vk::PhysicalDeviceLimits& limits, SharedFence& workCompleteFence, BufferMemoryTransferRequest* newBufferRequest, std::queue<std::unique_ptr<star::TransferManagerThread::InProcessRequestDependencies>>& inProcessRequests, size_t bufferIndexToUse, std::vector<vk::CommandBuffer>& commandBuffers, std::vector<SharedFence*>& commandBufferFences, std::unique_ptr<StarBuffer>* resultingBuffer) {
    assert(commandBufferFences[bufferIndexToUse] == nullptr && "Command buffer fence should have already been waited on and removed");
    
    auto transferSrcBuffer = std::make_unique<StarBuffer>(
        allocator, 
        newBufferRequest->getCreateArgs().instanceSize,
        newBufferRequest->getCreateArgs().instanceCount,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::SharingMode::eConcurrent,
        (newBufferRequest->getCreateArgs().useFlags & vk::BufferUsageFlagBits::eUniformBuffer) ? limits.minUniformBufferOffsetAlignment : 1
    );

    //check if the resulting buffer needs to be replaced or not 
    vk::DeviceSize resultSize = newBufferRequest->getCreateArgs().instanceSize * newBufferRequest->getCreateArgs().instanceCount;
    if (resultingBuffer->get() == nullptr || (resultingBuffer->get() != nullptr && resultSize > resultingBuffer->get()->getBufferSize())){ 
        auto newBuffer = std::make_unique<StarBuffer>(
            allocator, 
            newBufferRequest->getCreateArgs().instanceSize,
            newBufferRequest->getCreateArgs().instanceCount,
            newBufferRequest->getCreateArgs().creationFlags,
            newBufferRequest->getCreateArgs().memoryUsageFlags,
            newBufferRequest->getCreateArgs().useFlags | vk::BufferUsageFlagBits::eTransferDst,
            vk::SharingMode::eConcurrent, 
            (newBufferRequest->getCreateArgs().useFlags & vk::BufferUsageFlagBits::eUniformBuffer) ? limits.minUniformBufferOffsetAlignment : 1
        );   
        resultingBuffer->swap(newBuffer);   
    }

    newBufferRequest->writeData(*transferSrcBuffer);

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
    
        commandBuffer.copyBuffer(transferSrcBuffer->getVulkanBuffer(), resultingBuffer->get()->getVulkanBuffer(), copyRegion);
    }

    commandBuffer.end();

    {
        vk::SubmitInfo submitInfo{};
        submitInfo.pCommandBuffers = &commandBuffer; 
        submitInfo.commandBufferCount = 1;

        boost::unique_lock<boost::mutex> lock; 
        vk::Fence fence; 
        workCompleteFence.giveMeFence(lock, fence);

        auto commandResult = std::make_unique<vk::Result>(transferQueue.submit(1, &submitInfo, fence)); 

        if (*commandResult != vk::Result::eSuccess){
            //handle error
            std::runtime_error("Failed to submit transfer request"); 
        }
    }

    inProcessRequests.push(std::make_unique<InProcessRequestDependencies>(std::move(transferSrcBuffer), &workCompleteFence));
    commandBufferFences[bufferIndexToUse] = &workCompleteFence; 
}

void star::TransferManagerThread::checkForCleanups(vk::Device& device, std::queue<std::unique_ptr<star::TransferManagerThread::InProcessRequestDependencies>>& inProcessRequests, std::vector<SharedFence*>& commandBufferFences){
    
    std::queue<std::unique_ptr<InProcessRequestDependencies>> stillInProcess = std::queue<std::unique_ptr<InProcessRequestDependencies>>();

    while(!inProcessRequests.empty()){
        std::unique_ptr<InProcessRequestDependencies>& deps = inProcessRequests.front();
        {
            boost::unique_lock<boost::mutex> lock; 
            vk::Fence fence;
            deps->completeFence->giveMeFence(lock, fence);

            auto fenceResult = device.getFenceStatus(fence);
            if(fenceResult != vk::Result::eSuccess){
                stillInProcess.push(std::move(deps));
            }
        }

        inProcessRequests.pop();
    }

    if (stillInProcess.size() > 0)
    {
        while(!stillInProcess.empty()){
            inProcessRequests.push(std::move(stillInProcess.front()));
            stillInProcess.pop();
        }
    }
}

void star::TransferManagerThread::readyCommandBuffer(vk::Device& device, const size_t& indexSelected, std::vector<SharedFence*>& commandBufferFences){
    //sometimes multiple updates can be called on a buffer which shares a fence with the commandBuffers, if this happens, the manager is expected to reset the fence 
    if (commandBufferFences[indexSelected] != nullptr){
        {
            boost::unique_lock<boost::mutex> lock;
            vk::Fence fence; 
            commandBufferFences[indexSelected]->giveMeFence(lock, fence);
    
            auto result = device.waitForFences(fence, true, UINT64_MAX);
           
            if (result != vk::Result::eSuccess)
                throw std::runtime_error("Failed to wait for fence"); 
        }

        commandBufferFences[indexSelected] = nullptr; 
    }
}

star::TransferManagerThread::TransferManagerThread(star::StarDevice& device, star::Allocator& allocator, vk::PhysicalDeviceLimits deviceLimits, std::unique_ptr<star::StarQueueFamily> ownedQueue)
: device(device), allocator(allocator), transferQueue(std::move(ownedQueue)), 
commandBuffers(createCommandBuffers(device.getDevice(), transferQueue->getCommandPool(), 5)), commandBufferFences(std::vector<SharedFence*>(5)), deviceLimits(deviceLimits){

}

void star::TransferManagerThread::add(std::unique_ptr<star::TransferManagerThread::InterThreadRequest> request, const bool& isHighPriority){
    assert(!request->cpuWorkDoneByTransferThread->load() && "Request cannot be submitted if it is already in processing by worker");

    {
        boost::unique_lock<boost::mutex> lock;
        vk::Fence fence; 
        request->completeFence->giveMeFence(lock, fence);

        assert(this->device.getDevice().getFenceStatus(fence) == vk::Result::eSuccess && "Fences MUST be submitting in a signaled state to the worker");
    }
   
    //check if in use by any fence
    for (int i = 0; i < this->commandBufferFences.size(); i++){
        if (this->commandBufferFences[i] != nullptr && this->commandBufferFences[i] == request->completeFence){
            this->commandBufferFences[i] = VK_NULL_HANDLE;
        }
    }
    {
        boost::unique_lock<boost::mutex> lock;
        vk::Fence fence;
        request->completeFence->giveMeFence(lock, fence);
        this->device.getDevice().resetFences(std::vector<vk::Fence>{fence});
    }


    //TODO: will just need to manage removing elements from this when they are done
    this->transferRequests.emplace_back(std::move(request)); 

    if (this->thread.joinable()){
        if (isHighPriority){
            this->highPriorityRequests.value().push(this->transferRequests.back().get());
        }else{
            this->standardRequests.value().push(this->transferRequests.back().get());
        }
    }else{
        size_t targetBufferIndex = 0;
        if (this->previousBufferIndexUsed != this->commandBuffers.size()-1){
            targetBufferIndex = this->previousBufferIndexUsed + 1;
            this->previousBufferIndexUsed++; 
        }

        readyCommandBuffer(this->device.getDevice(), targetBufferIndex, this->commandBufferFences); 

        {
            createBuffer(this->device.getDevice(), this->allocator.get(), 
                this->transferQueue->getQueue(), this->deviceLimits, *this->transferRequests.back()->completeFence, 
                this->transferRequests.back()->bufferTransferRequest.get(), this->inProcessRequests, 
                targetBufferIndex, this->commandBuffers, this->commandBufferFences, this->transferRequests.back()->resultingBuffer.value());
        }

        this->previousBufferIndexUsed = targetBufferIndex; 
        this->transferRequests.back()->cpuWorkDoneByTransferThread->store(true);
        this->transferRequests.back().reset(); 
        this->transferRequests.pop_back();
    }
}

void star::TransferManagerThread::cleanup(){
    if (this->thread.joinable())
        checkForCleanups(this->device.getDevice(), this->inProcessRequests, this->commandBufferFences);

    bool fullCleanupAvailable = true;
    for (int i = 0; i < this->transferRequests.size(); i++){
        if (this->transferRequests[i]){
            fullCleanupAvailable = false; 

            {
                boost::unique_lock<boost::mutex> lock;
                vk::Fence fence; 
                this->transferRequests[i]->completeFence->giveMeFence(lock, fence);

                if (this->device.getDevice().getFenceStatus(fence) == vk::Result::eSuccess){
                    this->transferRequests[i].reset();
                }
            }
        }
    }

    if (fullCleanupAvailable){
        this->transferRequests = std::vector<std::unique_ptr<InterThreadRequest>>();
    }
}

star::TransferWorker::~TransferWorker() {
}

star::TransferWorker::TransferWorker(star::StarDevice& device, const bool& runAsync){

    if (device.doesHaveDedicatedFamily(star::Queue_Type::Ttransfer)){
        
    }

    this->threads.emplace_back(std::make_unique<TransferManagerThread>(device, device.getAllocator(), device.getPhysicalDevice().getProperties().limits, device.giveMeQueueFamily(star::Queue_Type::Ttransfer)));
    
    if (runAsync)
        this->threads.back()->startAsync();
}

void star::TransferWorker::add(std::unique_ptr<star::BufferMemoryTransferRequest> newBufferRequest, SharedFence& workCompleteFence, std::unique_ptr<star::StarBuffer>& resultingBuffer, boost::atomic<bool>& isBeingWorkedOnByTransferThread, const bool& isHighPriority){
    this->threads.back()->add(std::make_unique<TransferManagerThread::InterThreadRequest>(std::move(newBufferRequest), resultingBuffer, &workCompleteFence, &isBeingWorkedOnByTransferThread), isHighPriority);
}

void star::TransferWorker::update(){
    this->threads.back()->cleanup(); 
}