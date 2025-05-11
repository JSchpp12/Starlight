#include "TransferWorker.hpp"

#include "CastHelpers.hpp"

#include <thread>

star::TransferManagerThread::~TransferManagerThread(){
    if (this->thread.joinable()){
        this->stopAsync();
    }
}

void star::TransferManagerThread::startAsync(){
    this->shouldRun.store(true);
    this->thread = boost::thread(TransferManagerThread::mainLoop, 
        &this->shouldRun, 
        &this->device.getDevice(),
        this->transferQueue.get(), 
        &this->allocator.get(), 
        &this->deviceProperties,
        &this->commandBufferFences,
        &this->requestQueues
    );

    if (!this->thread.joinable())
        throw std::runtime_error("Failed to launch transfer thread");
}

void star::TransferManagerThread::stopAsync(){
    this->shouldRun.store(false);
            
    //wait for thread to exit
    this->thread.join();
}

void star::TransferManagerThread::mainLoop(boost::atomic<bool>* shouldRun, vk::Device* device, star::StarQueueFamily* transferQueue, VmaAllocator* allocator, const vk::PhysicalDeviceProperties* deviceProperties, std::vector<SharedFence*>* commandBufferFences, std::vector<boost::lockfree::stack<star::TransferManagerThread::InterThreadRequest*>*>* workingRequestQueues){
    std::cout << "Transfer thread started..." << std::endl;
    size_t targetBufferIndex = 0;
    size_t previousBufferIndexUsed = 0;
    std::vector<vk::CommandBuffer> commandBuffers = createCommandBuffers(*device, transferQueue->getCommandPool(), 5);
    std::queue<std::unique_ptr<TransferManagerThread::InProcessRequestDependencies>> inProcessRequests = std::queue<std::unique_ptr<TransferManagerThread::InProcessRequestDependencies>>();

    while(shouldRun->load()){
        InterThreadRequest* request = nullptr; 
        bool allEmpty = true; 

        //try to get a request
        for (int i = 0; i < workingRequestQueues->size(); i++){
            workingRequestQueues->at(i)->pop(request);
            if (request != nullptr){
                allEmpty = false; 
                break;
            }
        }

        if(request == nullptr && allEmpty){
            if (!inProcessRequests.empty()){
                checkForCleanups(*device, inProcessRequests, *commandBufferFences);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }else{
            if (previousBufferIndexUsed != commandBuffers.size()-1){
                targetBufferIndex = previousBufferIndexUsed + 1;
                previousBufferIndexUsed++; 
            }
    
            readyCommandBuffer(*device, targetBufferIndex, *commandBufferFences); 

            if (request->bufferTransferRequest){
                assert(request->resultingBuffer.has_value() && request->resultingBuffer.value() != nullptr && "Buffer request must contain both a request and a resulting address");

                createBuffer(*device, 
                    *allocator, 
                    *transferQueue, 
                    *deviceProperties, 
                    *request->completeFence,  
                    inProcessRequests, 
                    targetBufferIndex, 
                    commandBuffers, 
                    *commandBufferFences, 
                    request->bufferTransferRequest.get(),
                    request->resultingBuffer.value());   
            }else if (request->textureTransferRequest){
                assert(request->resultingTexture.has_value() && request->resultingTexture.value() != nullptr && "Texture request must contain both a request and a resulting address");
                
                createTexture(*device, 
                    *allocator, 
                    *transferQueue, 
                    *deviceProperties, 
                    *request->completeFence, 
                    inProcessRequests, 
                    targetBufferIndex, 
                    commandBuffers, 
                    *commandBufferFences, 
                    request->textureTransferRequest.get(), 
                    request->resultingTexture.value());
            }

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

void star::TransferManagerThread::createBuffer(vk::Device& device, VmaAllocator& allocator, StarQueueFamily& transferQueue, const vk::PhysicalDeviceProperties& deviceProperties, SharedFence& workCompleteFence, std::queue<std::unique_ptr<star::TransferManagerThread::InProcessRequestDependencies>>& inProcessRequests, const size_t& bufferIndexToUse, std::vector<vk::CommandBuffer>& commandBuffers, std::vector<SharedFence*>& commandBufferFences, TransferRequest::Buffer* newBufferRequest, std::unique_ptr<StarBuffer>* resultingBuffer) {
    assert(commandBufferFences[bufferIndexToUse] == nullptr && "Command buffer fence should have already been waited on and removed");

    auto transferSrcBuffer = newBufferRequest->createStagingBuffer(device, allocator, transferQueue.getQueueFamilyIndex()); 

    auto newResult = newBufferRequest->createFinal(device, allocator, transferQueue.getQueueFamilyIndex()); 
    resultingBuffer->swap(newResult); 

    newBufferRequest->writeDataToStageBuffer(*transferSrcBuffer);

    //copy operations
    vk::CommandBuffer& commandBuffer = commandBuffers[bufferIndexToUse];
    {
        vk::CommandBufferBeginInfo beginInfo{};
        beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    
        commandBuffer.begin(beginInfo);
    }

    newBufferRequest->copyFromTransferSRCToDST(*transferSrcBuffer, *resultingBuffer->get(), commandBuffer);

    commandBuffer.end();

    {
        vk::SubmitInfo submitInfo{};
        submitInfo.pCommandBuffers = &commandBuffer; 
        submitInfo.commandBufferCount = 1;

        boost::unique_lock<boost::mutex> lock; 
        vk::Fence* fence = nullptr; 
        workCompleteFence.giveMeResource(lock, fence);

        auto commandResult = std::make_unique<vk::Result>(transferQueue.getQueue().submit(1, &submitInfo, *fence)); 

        if (*commandResult != vk::Result::eSuccess){
            //handle error
            std::runtime_error("Failed to submit transfer request"); 
        }
    }

    inProcessRequests.push(std::make_unique<InProcessRequestDependencies>(std::move(transferSrcBuffer), &workCompleteFence));
    commandBufferFences[bufferIndexToUse] = &workCompleteFence; 
}

void star::TransferManagerThread::createTexture(vk::Device& device, VmaAllocator& allocator, StarQueueFamily& transferQueue, const vk::PhysicalDeviceProperties& deviceProperties, SharedFence& workCompleteFence, std::queue<std::unique_ptr<InProcessRequestDependencies>>& inProcessRequests, const size_t& bufferIndexToUse, std::vector<vk::CommandBuffer>& commandBuffers, std::vector<SharedFence*>& commandBufferFences, star::TransferRequest::Texture* newTextureRequest, std::unique_ptr<star::StarTexture>* resultingTexture){
    
    auto transferSrcBuffer = newTextureRequest->createStagingBuffer(device, allocator, transferQueue.getQueueFamilyIndex()); 

    //should eventually implement option to jsut re-use existing image
    bool newImageCreated = true; 

    auto finalTexture = newTextureRequest->createFinal(device, allocator, transferQueue.getQueueFamilyIndex()); 
    resultingTexture->swap(finalTexture);

    newTextureRequest->writeDataToStageBuffer(*transferSrcBuffer);

    //transition image layout
    if (!newImageCreated){
        //Not sure how to manage vulkan images since we wont know what layout they might be in at this point
        throw std::runtime_error("unsupported image operation in transfer manager");
    }

    //copy operations
    vk::CommandBuffer& commandBuffer = commandBuffers[bufferIndexToUse];
    {
        vk::CommandBufferBeginInfo beginInfo{};
        beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    
        commandBuffer.begin(beginInfo);
    }

    newTextureRequest->copyFromTransferSRCToDST(*transferSrcBuffer, *resultingTexture->get(), commandBuffer); 

    commandBuffer.end();

    {
        vk::SubmitInfo submitInfo{};
        submitInfo.pCommandBuffers = &commandBuffer; 
        submitInfo.commandBufferCount = 1;

        boost::unique_lock<boost::mutex> lock; 
        vk::Fence* fence = nullptr; 
        workCompleteFence.giveMeResource(lock, fence);

        auto commandResult = std::make_unique<vk::Result>(transferQueue.getQueue().submit(1, &submitInfo, *fence)); 

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
            vk::Fence* fence = nullptr;
            deps->completeFence->giveMeResource(lock, fence);

            auto fenceResult = device.getFenceStatus(*fence);
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
            vk::Fence* fence = nullptr; 
            commandBufferFences[indexSelected]->giveMeResource(lock, fence);
    
            auto result = device.waitForFences(*fence, true, UINT64_MAX);
           
            if (result != vk::Result::eSuccess)
                throw std::runtime_error("Failed to wait for fence"); 
        }

        commandBufferFences[indexSelected] = nullptr; 
    }
}

star::TransferManagerThread::TransferManagerThread(star::StarDevice& device, star::Allocator& allocator,  std::vector<boost::lockfree::stack<InterThreadRequest*>*> requestQueues, const vk::PhysicalDeviceProperties& deviceProperties, std::unique_ptr<star::StarQueueFamily> ownedQueue)
: device(device), allocator(allocator), transferQueue(std::move(ownedQueue)), requestQueues(requestQueues), commandBuffers(createCommandBuffers(device.getDevice(), transferQueue->getCommandPool(), 5)), commandBufferFences(std::vector<SharedFence*>(5)), deviceProperties(deviceProperties){

}

void star::TransferManagerThread::cleanup(){
    if (this->thread.joinable())
        checkForCleanups(this->device.getDevice(), this->inProcessRequests, this->commandBufferFences);
}

bool star::TransferManagerThread::isFenceInUse(const SharedFence& fence, const bool& clearIfFound){
        //check if in use by any fence
        for (int i = 0; i < this->commandBufferFences.size(); i++){
            if (this->commandBufferFences[i] != nullptr && this->commandBufferFences[i] == &fence){
                if (clearIfFound)
                    this->commandBufferFences[i] = VK_NULL_HANDLE;
                return true;
            }
        }

        return false;
}

star::TransferWorker::~TransferWorker() {
}

star::TransferWorker::TransferWorker(star::StarDevice& device, bool overrideToSingleThreadMode) : device(device){
    bool runAsync = !overrideToSingleThreadMode;
    if (!device.doesHaveDedicatedFamily(star::Queue_Type::Ttransfer)){
        runAsync = false;
    }

    std::vector<std::unique_ptr<StarQueueFamily>> useableQueues; 
    bool done = false;
    while(!done){

        //really only want to expect 2 possible transfer threads
        if (useableQueues.size() > 1){
            break;
        }

        auto possibleQueue = device.giveMeQueueFamily(star::Queue_Type::Ttransfer); 
        if (possibleQueue){
            useableQueues.emplace_back(std::move(possibleQueue)); 
        }else{
            break;
        }
    }

    if (useableQueues.size() == 2){
        this->threads.emplace_back(std::make_unique<TransferManagerThread>(
            device, 
            device.getAllocator(),  
            std::vector<boost::lockfree::stack<TransferManagerThread::InterThreadRequest*>*>{&this->highPriorityRequests}, 
            device.getPhysicalDevice().getProperties(), 
            std::move(useableQueues[0])));
        this->threads.emplace_back(std::make_unique<TransferManagerThread>(
            device, 
            device.getAllocator(),  
            std::vector<boost::lockfree::stack<TransferManagerThread::InterThreadRequest*>*>{&this->standardRequests}, 
            device.getPhysicalDevice().getProperties(), 
            std::move(useableQueues[1])));

    }else if (useableQueues.size() == 1){
        this->threads.emplace_back(std::make_unique<TransferManagerThread>(
            device, 
            device.getAllocator(),  
            std::vector<boost::lockfree::stack<TransferManagerThread::InterThreadRequest*>*>{&this->highPriorityRequests, &this->standardRequests}, 
            device.getPhysicalDevice().getProperties(), 
            std::move(useableQueues[0])));
    }else{
        throw std::runtime_error("Needs at least 1 dedicated queue -- single threaded mode not supported yet"); 
    }

    if (this->threads.size() == 0)
        throw std::runtime_error("Failed to create transfer worker");

    for (auto& thread : this->threads)
        thread->startAsync(); 
}

void star::TransferWorker::add(SharedFence& workCompleteFence, boost::atomic<bool>& isBeingWorkedOnByTransferThread, std::unique_ptr<TransferRequest::Buffer> newBufferRequest, std::unique_ptr<star::StarBuffer>& resultingBuffer, const bool& isHighPriority){
    
    auto newRequest = std::make_unique<TransferManagerThread::InterThreadRequest>(&isBeingWorkedOnByTransferThread, &workCompleteFence, std::move(newBufferRequest), resultingBuffer);
    
    checkFenceStatus(*newRequest); 
    insertRequest(std::move(newRequest), isHighPriority); 
}

void star::TransferWorker::add(SharedFence& workCompleteFence, boost::atomic<bool>& isBeingWorkedOnByTransferThread, std::unique_ptr<star::TransferRequest::Texture> newTextureRequest, std::unique_ptr<StarTexture>& resultingTexture, const bool& isHighPriority){
    auto newRequest = std::make_unique<TransferManagerThread::InterThreadRequest>(&isBeingWorkedOnByTransferThread, &workCompleteFence, std::move(newTextureRequest), resultingTexture);
    
    checkFenceStatus(*newRequest); 
    insertRequest(std::move(newRequest), isHighPriority); 
}

void star::TransferWorker::update(){
    for (auto& thread : this->threads)
        thread->cleanup();
}

void star::TransferWorker::checkFenceStatus(TransferManagerThread::InterThreadRequest& request){
    {
        boost::unique_lock<boost::mutex> lock;
        vk::Fence* fence = nullptr; 
        request.completeFence->giveMeResource(lock, fence);

        assert(this->device.getDevice().getFenceStatus(*fence) == vk::Result::eSuccess && "Fences MUST be submitting in a signaled state to the worker");
    }

    for(auto& thread : this->threads){
        if(thread->isFenceInUse(*request.completeFence, true))
            break;
    }

    //check if in use by threads
    {
        boost::unique_lock<boost::mutex> lock;
        vk::Fence* fence = nullptr;
        request.completeFence->giveMeResource(lock, fence);
        this->device.getDevice().resetFences(std::vector<vk::Fence>{*fence});
    }
}

void star::TransferWorker::insertRequest(std::unique_ptr<TransferManagerThread::InterThreadRequest> newRequest, const bool& isHighPriority){
    this->requests.push_back(std::move(newRequest)); 

    if (isHighPriority){
        this->highPriorityRequests.push(this->requests.back().get()); 
    }else{
        this->standardRequests.push(this->requests.back().get()); 
    }
}

void star::TransferWorker::checkForCleanups(){
    bool fullCleanupAvailable = true;
    for (int i = 0; i < this->requests.size(); i++){
        if (this->requests[i]){
            fullCleanupAvailable = false; 

            {
                boost::unique_lock<boost::mutex> lock;
                vk::Fence* fence = nullptr; 
                this->requests[i]->completeFence->giveMeResource(lock, fence);

                if (this->device.getDevice().getFenceStatus(*fence) == vk::Result::eSuccess){
                    this->requests[i].reset();
                }
            }
        }
    }

    if (fullCleanupAvailable){
        this->requests = std::vector<std::unique_ptr<TransferManagerThread::InterThreadRequest>>();
    }
}