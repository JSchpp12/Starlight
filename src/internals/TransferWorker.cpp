#include "TransferWorker.hpp"

#include "CastHelpers.hpp"

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
        &this->transferQueue->getCommandPool(),
        &this->transferQueue->getQueue(), 
        &this->allocator.get(), 
        &this->deviceProperties,
        &this->commandBufferFences,
        &this->highPriorityRequests, 
        &this->standardPriorityRequests
    );

    if (!this->thread.joinable())
        throw std::runtime_error("Failed to launch transfer thread");
}

void star::TransferManagerThread::stopAsync(){
    this->shouldRun.store(false);
            
    //wait for thread to exit
    this->thread.join();
}

void star::TransferManagerThread::mainLoop(boost::atomic<bool>* shouldRun, vk::Device* device, vk::CommandPool* transferPool, vk::Queue* transferQueue, VmaAllocator* allocator, const vk::PhysicalDeviceProperties* deviceProperties, std::vector<SharedFence*>* commandBufferFences, boost::lockfree::stack<star::TransferManagerThread::InterThreadRequest*>* highPriorityRequests, boost::lockfree::stack<star::TransferManagerThread::InterThreadRequest*>* standardRequests){
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

                request->bufferTransferRequest.get()->afterWriteData();
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

void star::TransferManagerThread::transitionImageLayout(vk::Image &image, vk::CommandBuffer &commandBuffer, const vk::Format &format, const vk::ImageLayout &oldLayout, const vk::ImageLayout &newLayout)
{
	//create a barrier to prevent pipeline from moving forward until image transition is complete
	vk::ImageMemoryBarrier barrier{};
	barrier.sType = vk::StructureType::eImageMemoryBarrier;     //specific flag for image operations
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	//if barrier is used for transferring ownership between queue families, this would be important -- set to ignore since we are not doing this
	barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
	barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseMipLevel = 0;                          //image does not have any mipmap levels
	barrier.subresourceRange.levelCount = 1;                            //image is not an array
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	//the operations that need to be completed before and after the barrier, need to be defined
	barrier.srcAccessMask = {}; //TODO
	barrier.dstAccessMask = {}; //TODO

	vk::PipelineStageFlags sourceStage, destinationStage;

	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
		//undefined transition state, dont need to wait for this to complete
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && (newLayout == vk::ImageLayout::eShaderReadOnlyOptimal || newLayout == vk::ImageLayout::eGeneral)) {
		//transfer destination shader reading, will need to wait for completion. Especially in the frag shader where reads will happen
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eNone;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eBottomOfPipe;
	}
	// else if (oldLayout == vk::ImageLayout::eShaderReadOnlyOptimal && newLayout == vk::ImageLayout::eTransferDstOptimal) {
	// 	//preparing to update texture during runtime, need to wait for top of pipe
	// 	//barrier.srcAccessMask = vk::AccessFlagBits::;
	// 	barrier.srcAccessMask = {}; 
	// 	barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead; 

	// 	sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
	// 	destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	// }
	else{
		throw std::invalid_argument("unsupported layout transition!");
	}

	//transfer writes must occurr during the pipeline transfer stage
	commandBuffer.pipelineBarrier(
		sourceStage,                        //which pipeline stages should occurr before barrier 
		destinationStage,                   //pipeline stage in which operations will wait on the barrier 
		{},
		{},
		nullptr,
		barrier
	);
}

void star::TransferManagerThread::createBuffer(vk::Device& device, VmaAllocator& allocator, vk::Queue& transferQueue, const vk::PhysicalDeviceProperties& deviceProperties, SharedFence& workCompleteFence, std::queue<std::unique_ptr<star::TransferManagerThread::InProcessRequestDependencies>>& inProcessRequests, const size_t& bufferIndexToUse, std::vector<vk::CommandBuffer>& commandBuffers, std::vector<SharedFence*>& commandBufferFences, TransferRequest::Memory<StarBuffer::BufferCreationArgs>* newBufferRequest, std::unique_ptr<StarBuffer>* resultingBuffer) {
    assert(commandBufferFences[bufferIndexToUse] == nullptr && "Command buffer fence should have already been waited on and removed");
    
    auto createArgs = newBufferRequest->getCreateArgs(deviceProperties);

    auto transferSrcBuffer = std::make_unique<StarBuffer>(
        allocator, 
        createArgs.instanceSize,
        createArgs.instanceCount,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::SharingMode::eConcurrent,
        createArgs.allocationName + "_TransferSRCBuffer",
        (createArgs.useFlags & vk::BufferUsageFlagBits::eUniformBuffer) ? deviceProperties.limits.minUniformBufferOffsetAlignment : 1
    );

    //check if the resulting buffer needs to be replaced or not 
    vk::DeviceSize resultSize = createArgs.instanceSize * createArgs.instanceCount;
    if (resultingBuffer->get() == nullptr || (resultingBuffer->get() != nullptr && resultSize > resultingBuffer->get()->getBufferSize())){ 
        auto newBuffer = std::make_unique<StarBuffer>(
            allocator, 
            createArgs.instanceSize,
            createArgs.instanceCount,
            createArgs.creationFlags,
            createArgs.memoryUsageFlags,
            createArgs.useFlags | vk::BufferUsageFlagBits::eTransferDst,
            vk::SharingMode::eConcurrent, 
            createArgs.allocationName,
            (createArgs.useFlags & vk::BufferUsageFlagBits::eUniformBuffer) ? deviceProperties.limits.minUniformBufferOffsetAlignment : 1
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

void star::TransferManagerThread::createTexture(vk::Device& device, VmaAllocator& allocator, vk::Queue& transferQueue, const vk::PhysicalDeviceProperties& deviceProperties, SharedFence& workCompleteFence, std::queue<std::unique_ptr<InProcessRequestDependencies>>& inProcessRequests, const size_t& bufferIndexToUse, std::vector<vk::CommandBuffer>& commandBuffers, std::vector<SharedFence*>& commandBufferFences, star::TransferRequest::Memory<star::StarTexture::TextureCreateSettings>* newTextureRequest, std::unique_ptr<star::StarTexture>* resultingTexture){
    StarTexture::TextureCreateSettings createArgs = newTextureRequest->getCreateArgs(deviceProperties);
    createArgs.imageUsage |= vk::ImageUsageFlagBits::eTransferDst;

    bool newImageCreated = false; 
    
    vk::DeviceSize size = createArgs.height * createArgs.width * createArgs.channels * createArgs.depth * createArgs.byteDepth;
    {
        newImageCreated = true; 

        auto finalTexture = std::make_unique<StarTexture>(createArgs, device, allocator);
        resultingTexture->swap(finalTexture);
    }

    auto transferSrcBuffer = std::make_unique<StarBuffer>(
        allocator,
        size, 
        1,
        VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT | VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::SharingMode::eConcurrent,
        createArgs.allocationName + "_TransferSRCBuffer"
    );

    newTextureRequest->writeData(*transferSrcBuffer);

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
    {
        auto vulkanImage = resultingTexture->get()->getImage();
    
        transitionImageLayout(vulkanImage, commandBuffer, createArgs.imageFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    
        {
            vk::BufferImageCopy region{}; 
            region.bufferOffset = 0; 
            region.bufferRowLength = 0; 
            region.bufferImageHeight = 0; 
    
            region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor; 
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = vk::Offset3D{}; 
            region.imageExtent = vk::Extent3D{
                CastHelpers::int_to_unsigned_int(createArgs.width),
                CastHelpers::int_to_unsigned_int(createArgs.height), 
                1
            };
    
            commandBuffer.copyBufferToImage(transferSrcBuffer->getVulkanBuffer(), resultingTexture->get()->getImage(), vk::ImageLayout::eTransferDstOptimal, region);
        }
    
        transitionImageLayout(vulkanImage, commandBuffer, createArgs.imageFormat, vk::ImageLayout::eTransferDstOptimal, createArgs.initialLayout);
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

star::TransferManagerThread::TransferManagerThread(star::StarDevice& device, star::Allocator& allocator, boost::lockfree::stack<InterThreadRequest*>& highPriorityRequests, boost::lockfree::stack<InterThreadRequest*>& standardPriorityRequests, const vk::PhysicalDeviceProperties& deviceProperties, std::unique_ptr<star::StarQueueFamily> ownedQueue)
: device(device), allocator(allocator), transferQueue(std::move(ownedQueue)), standardPriorityRequests(standardPriorityRequests), highPriorityRequests(highPriorityRequests), commandBuffers(createCommandBuffers(device.getDevice(), transferQueue->getCommandPool(), 5)), commandBufferFences(std::vector<SharedFence*>(5)), deviceProperties(deviceProperties){

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

    bool done = false;
    while(!done){

        //really only want to expect 2 possible transfer threads
        if (this->threads.size() > 1){
            break;
        }

        auto possibleQueue = device.giveMeQueueFamily(star::Queue_Type::Ttransfer); 
        if (possibleQueue){
            this->threads.emplace_back(std::make_unique<TransferManagerThread>(device, device.getAllocator(), this->highPriorityRequests, this->standardRequests, device.getPhysicalDevice().getProperties(), std::move(possibleQueue)));
        }else{
            break;
        }
    }

    if (this->threads.size() == 0)
        throw std::runtime_error("Failed to create transfer worker");

    for (auto& thread : this->threads)
        thread->startAsync(); 
}

void star::TransferWorker::add(SharedFence& workCompleteFence, boost::atomic<bool>& isBeingWorkedOnByTransferThread, std::unique_ptr<TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>> newBufferRequest, std::unique_ptr<star::StarBuffer>& resultingBuffer, const bool& isHighPriority){
    
    auto newRequest = std::make_unique<TransferManagerThread::InterThreadRequest>(&isBeingWorkedOnByTransferThread, &workCompleteFence, std::move(newBufferRequest), resultingBuffer);
    
    checkFenceStatus(*newRequest); 
    insertRequest(std::move(newRequest), isHighPriority); 
}

void star::TransferWorker::add(SharedFence& workCompleteFence, boost::atomic<bool>& isBeingWorkedOnByTransferThread, std::unique_ptr<star::TransferRequest::Memory<star::StarTexture::TextureCreateSettings>> newTextureRequest, std::unique_ptr<StarTexture>& resultingTexture, const bool& isHighPriority){
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
        vk::Fence fence; 
        request.completeFence->giveMeFence(lock, fence);

        assert(this->device.getDevice().getFenceStatus(fence) == vk::Result::eSuccess && "Fences MUST be submitting in a signaled state to the worker");
    }

    for(auto& thread : this->threads){
        if(thread->isFenceInUse(*request.completeFence, true))
            break;
    }

    //check if in use by threads
    {
        boost::unique_lock<boost::mutex> lock;
        vk::Fence fence;
        request.completeFence->giveMeFence(lock, fence);
        this->device.getDevice().resetFences(std::vector<vk::Fence>{fence});
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
                vk::Fence fence; 
                this->requests[i]->completeFence->giveMeFence(lock, fence);

                if (this->device.getDevice().getFenceStatus(fence) == vk::Result::eSuccess){
                    this->requests[i].reset();
                }
            }
        }
    }

    if (fullCleanupAvailable){
        this->requests = std::vector<std::unique_ptr<TransferManagerThread::InterThreadRequest>>();
    }
}