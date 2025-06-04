#include "TransferWorker.hpp"

#include "CastHelpers.hpp"

#include <thread>

star::TransferManagerThread::~TransferManagerThread()
{
    if (this->thread.joinable())
    {
        this->stopAsync();
    }
}

void star::TransferManagerThread::startAsync()
{
    this->shouldRun.store(true);
    this->thread = boost::thread(TransferManagerThread::mainLoop,
                                 SubThreadInfo(&this->shouldRun, this->device.getDevice(), this->myCommandPool,
                                               this->myQueues, this->allocator, this->deviceProperties,
                                               &this->requestQueues, this->allTransferQueueFamilyIndicesInUse));

    if (!this->thread.joinable())
        throw std::runtime_error("Failed to launch transfer thread");
}

void star::TransferManagerThread::stopAsync()
{
    this->shouldRun.store(false);

    // wait for thread to exit
    this->thread.join();
}

void star::TransferManagerThread::mainLoop(TransferManagerThread::SubThreadInfo myInfo)
{
    std::cout << "Transfer thread started..." << std::endl;
    size_t targetBufferIndex = 0;
    size_t previousBufferIndexUsed = 0;
    std::queue<std::unique_ptr<TransferManagerThread::InProcessRequestDependencies>> inProcessRequests =
        std::queue<std::unique_ptr<TransferManagerThread::InProcessRequestDependencies>>();

    // need to make command buffers
    std::vector<SharedFence *> commandBufferFences = std::vector<SharedFence *>(20);
    StarCommandBuffer commandBuffers =
        StarCommandBuffer(myInfo.device, 20, myInfo.commandPool, star::Queue_Type::Ttransfer, false, false);

    while (myInfo.shouldRun->load())
    {
        InterThreadRequest *request = nullptr;
        bool allEmpty = true;

        // try to get a request
        for (int i = 0; i < myInfo.workingRequestQueues->size(); i++)
        {
            myInfo.workingRequestQueues->at(i)->pop(request);
            if (request != nullptr)
            {
                allEmpty = false;
                break;
            }
        }

        if (request == nullptr && allEmpty)
        {
            if (!inProcessRequests.empty())
            {
                checkForCleanups(myInfo.device, inProcessRequests);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        else
        {
            if (previousBufferIndexUsed != commandBuffers.getNumBuffers() - 1)
            {
                targetBufferIndex = previousBufferIndexUsed + 1;
                previousBufferIndexUsed++;
            }

            readyCommandBuffer(myInfo.device, targetBufferIndex, commandBufferFences);

            if (request->bufferTransferRequest)
            {
                assert(request->resultingBuffer.has_value() && request->resultingBuffer.value() != nullptr &&
                       "Buffer request must contain both a request and a resulting address");

                createBuffer(myInfo.device, myInfo.allocator, myInfo.queues.at(0), myInfo.deviceProperties,
                             *request->completeFence, inProcessRequests, targetBufferIndex,
                             myInfo.allTransferQueueFamilyIndicesInUse, commandBuffers, commandBufferFences,
                             request->bufferTransferRequest.get(), request->resultingBuffer.value());
            }
            else if (request->textureTransferRequest)
            {
                assert(request->resultingTexture.has_value() && request->resultingTexture.value() != nullptr &&
                       "Texture request must contain both a request and a resulting address");

                createTexture(myInfo.device, myInfo.allocator, myInfo.queues.at(0), myInfo.deviceProperties,
                              *request->completeFence, inProcessRequests, targetBufferIndex,
                              myInfo.allTransferQueueFamilyIndicesInUse, commandBuffers, commandBufferFences,
                              request->textureTransferRequest.get(), request->resultingTexture.value());
            }

            request->cpuWorkDoneByTransferThread->store(true);
            request->cpuWorkDoneByTransferThread->notify_all();
        }
    }

    std::cout << "Transfer Thread exiting..." << std::endl;
}

std::unique_ptr<star::StarCommandBuffer> star::TransferManagerThread::CreateCommandBuffers(
    vk::Device &device, std::shared_ptr<star::StarCommandPool> commandPool, const uint8_t &numToCreate)
{
    return std::make_unique<StarCommandBuffer>(device, numToCreate, commandPool, star::Queue_Type::Ttransfer, false,
                                               false);
}

void star::TransferManagerThread::createBuffer(
    vk::Device &device, VmaAllocator &allocator, StarQueue &transferQueue,
    const vk::PhysicalDeviceProperties &deviceProperties, SharedFence &workCompleteFence,
    std::queue<std::unique_ptr<star::TransferManagerThread::InProcessRequestDependencies>> &inProcessRequests,
    const size_t &bufferIndexToUse, const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
    StarCommandBuffer &commandBuffers, std::vector<SharedFence *> &commandBufferFences,
    TransferRequest::Buffer *newBufferRequest, std::unique_ptr<StarBuffer> *resultingBuffer)
{
    assert(commandBufferFences[bufferIndexToUse] == nullptr &&
           "Command buffer fence should have already been waited on and removed");

    auto transferSrcBuffer = newBufferRequest->createStagingBuffer(device, allocator);

    {
        auto newResult = newBufferRequest->createFinal(device, allocator, allTransferQueueFamilyIndicesInUse);
        if (!*resultingBuffer ||
            (resultingBuffer->get() && newResult->getBufferSize() > resultingBuffer->get()->getBufferSize()))
        {
            resultingBuffer->swap(newResult);
        }
    }

    newBufferRequest->writeDataToStageBuffer(*transferSrcBuffer);

    // copy operations
    {
        vk::CommandBufferBeginInfo beginInfo{};
        beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;

        commandBuffers.begin(bufferIndexToUse, beginInfo);
    }

    newBufferRequest->copyFromTransferSRCToDST(*transferSrcBuffer, *resultingBuffer->get(),
                                               commandBuffers.buffer(bufferIndexToUse));

    commandBuffers.buffer(bufferIndexToUse).end();

    {
        boost::unique_lock<boost::mutex> lock;
        vk::Fence *fence = nullptr;
        workCompleteFence.giveMeResource(lock, fence);

        auto submitInfo = commandBuffers.getFinalizedSubmitInfo(bufferIndexToUse).getVulkanSubmitInfo();
        auto commandResult =
            std::make_unique<vk::Result>(transferQueue.getVulkanQueue().submit(1, &submitInfo, *fence));

        if (*commandResult != vk::Result::eSuccess)
        {
            // handle error
            std::runtime_error("Failed to submit transfer request");
        }
    }

    inProcessRequests.push(
        std::make_unique<InProcessRequestDependencies>(std::move(transferSrcBuffer), &workCompleteFence));
    commandBufferFences[bufferIndexToUse] = &workCompleteFence;
}

void star::TransferManagerThread::createTexture(
    vk::Device &device, VmaAllocator &allocator, StarQueue &queue, const vk::PhysicalDeviceProperties &deviceProperties,
    SharedFence &workCompleteFence, std::queue<std::unique_ptr<InProcessRequestDependencies>> &inProcessRequests,
    const size_t &bufferIndexToUse, const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
    StarCommandBuffer &commandBuffers, std::vector<SharedFence *> &commandBufferFences,
    star::TransferRequest::Texture *newTextureRequest, std::unique_ptr<star::StarTexture> *resultingTexture)
{

    auto transferSrcBuffer = newTextureRequest->createStagingBuffer(device, allocator);

    // should eventually implement option to jsut re-use existing image
    bool newImageCreated = true;

    auto finalTexture = newTextureRequest->createFinal(device, allocator, allTransferQueueFamilyIndicesInUse);
    resultingTexture->swap(finalTexture);

    newTextureRequest->writeDataToStageBuffer(*transferSrcBuffer);

    // transition image layout
    if (!newImageCreated)
    {
        // Not sure how to manage vulkan images since we wont know what layout they might be in at this point
        throw std::runtime_error("unsupported image operation in transfer manager");
    }

    // copy operations
    {
        vk::CommandBufferBeginInfo beginInfo{};
        beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;

        commandBuffers.begin(bufferIndexToUse, beginInfo);
    }

    newTextureRequest->copyFromTransferSRCToDST(*transferSrcBuffer, *resultingTexture->get(),
                                                commandBuffers.buffer(bufferIndexToUse));

    commandBuffers.buffer(bufferIndexToUse).end();

    {
        auto submitInfo = commandBuffers.getFinalizedSubmitInfo(bufferIndexToUse).getVulkanSubmitInfo();

        boost::unique_lock<boost::mutex> lock;
        vk::Fence *fence = nullptr;
        workCompleteFence.giveMeResource(lock, fence);

        auto commandResult = std::make_unique<vk::Result>(queue.getVulkanQueue().submit(1, &submitInfo, *fence));

        if (*commandResult != vk::Result::eSuccess)
        {
            // handle error
            std::runtime_error("Failed to submit transfer request");
        }
    }

    inProcessRequests.push(
        std::make_unique<InProcessRequestDependencies>(std::move(transferSrcBuffer), &workCompleteFence));
    commandBufferFences[bufferIndexToUse] = &workCompleteFence;
}

void star::TransferManagerThread::checkForCleanups(
    vk::Device &device,
    std::queue<std::unique_ptr<star::TransferManagerThread::InProcessRequestDependencies>> &inProcessRequests)
{

    std::queue<std::unique_ptr<InProcessRequestDependencies>> stillInProcess =
        std::queue<std::unique_ptr<InProcessRequestDependencies>>();

    while (!inProcessRequests.empty())
    {
        std::unique_ptr<InProcessRequestDependencies> &deps = inProcessRequests.front();
        {
            boost::unique_lock<boost::mutex> lock;
            vk::Fence *fence = nullptr;
            deps->completeFence->giveMeResource(lock, fence);

            auto fenceResult = device.getFenceStatus(*fence);
            if (fenceResult != vk::Result::eSuccess)
            {
                stillInProcess.push(std::move(deps));
            }
        }

        inProcessRequests.pop();
    }

    if (stillInProcess.size() > 0)
    {
        while (!stillInProcess.empty())
        {
            inProcessRequests.push(std::move(stillInProcess.front()));
            stillInProcess.pop();
        }
    }
}

void star::TransferManagerThread::readyCommandBuffer(vk::Device &device, const size_t &indexSelected,
                                                     std::vector<SharedFence *> &commandBufferFences)
{
    // sometimes multiple updates can be called on a buffer which shares a fence with the commandBuffers, if this
    // happens, the manager is expected to reset the fence
    if (commandBufferFences[indexSelected] != nullptr)
    {
        {
            boost::unique_lock<boost::mutex> lock;
            vk::Fence *fence = nullptr;
            commandBufferFences[indexSelected]->giveMeResource(lock, fence);

            auto result = device.waitForFences(*fence, true, UINT64_MAX);

            if (result != vk::Result::eSuccess)
                throw std::runtime_error("Failed to wait for fence");
        }

        commandBufferFences[indexSelected] = nullptr;
    }
}

star::TransferManagerThread::TransferManagerThread(
    star::StarDevice &device, VmaAllocator &allocator,
    std::vector<boost::lockfree::stack<InterThreadRequest *> *> requestQueues,
    const vk::PhysicalDeviceProperties &deviceProperties, std::vector<StarQueue> myQueues, StarQueueFamily &familyToUse,
    const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse)
    : device(device), allocator(allocator), myQueues(myQueues), requestQueues(requestQueues),
      myCommandPool(std::make_shared<StarCommandPool>(device.getDevice(), familyToUse.getQueueFamilyIndex(), true)),
      deviceProperties(deviceProperties), familyToUse(familyToUse),
      allTransferQueueFamilyIndicesInUse(allTransferQueueFamilyIndicesInUse)
{
}

void star::TransferManagerThread::cleanup()
{
    if (this->thread.joinable())
        checkForCleanups(this->device.getDevice(), this->inProcessRequests);
}

star::TransferWorker::~TransferWorker()
{
}

star::TransferWorker::TransferWorker(star::StarDevice &device, bool overrideToSingleThreadMode) : device(device)
{
    bool runAsync = !overrideToSingleThreadMode;
    if (!device.doesHaveDedicatedFamily(star::Queue_Type::Ttransfer))
    {
        runAsync = false;
    }

    // grab all the queue families which can be used for transfer operations
    int targetNumFamilies = 2;
    std::vector<std::unique_ptr<StarQueueFamily>> foundFams = std::vector<std::unique_ptr<StarQueueFamily>>();

    for (int i = 0; i < targetNumFamilies; i++)
    {
        auto newFamily = device.giveMeQueueFamily(star::Queue_Type::Ttransfer);
        if (newFamily == nullptr)
        {
            break;
        }

        this->myQueueFamilies.push_back(std::move(newFamily));
    }

    this->threads = CreateThreads(device, this->myQueueFamilies, this->highPriorityRequests, this->standardRequests);

    if (this->threads.size() == 0)
        throw std::runtime_error("Failed to create transfer worker");

    for (auto &thread : this->threads)
        thread->startAsync();
}

void star::TransferWorker::add(SharedFence &workCompleteFence, boost::atomic<bool> &isBeingWorkedOnByTransferThread,
                               std::unique_ptr<TransferRequest::Buffer> newBufferRequest,
                               std::unique_ptr<star::StarBuffer> &resultingBuffer, const bool &isHighPriority)
{
    auto newRequest = std::make_unique<TransferManagerThread::InterThreadRequest>(
        &isBeingWorkedOnByTransferThread, &workCompleteFence, std::move(newBufferRequest), resultingBuffer);

    checkFenceStatus(*newRequest);
    insertRequest(std::move(newRequest), isHighPriority);
}

void star::TransferWorker::add(SharedFence &workCompleteFence, boost::atomic<bool> &isBeingWorkedOnByTransferThread,
                               std::unique_ptr<star::TransferRequest::Texture> newTextureRequest,
                               std::unique_ptr<StarTexture> &resultingTexture, const bool &isHighPriority)
{
    auto newRequest = std::make_unique<TransferManagerThread::InterThreadRequest>(
        &isBeingWorkedOnByTransferThread, &workCompleteFence, std::move(newTextureRequest), resultingTexture);

    checkFenceStatus(*newRequest);
    insertRequest(std::move(newRequest), isHighPriority);
}

void star::TransferWorker::update()
{
    for (auto &thread : this->threads)
    {
        thread->cleanup();
    }
}

void star::TransferWorker::checkFenceStatus(TransferManagerThread::InterThreadRequest &request)
{
    {
        boost::unique_lock<boost::mutex> lock;
        vk::Fence *fence = nullptr;
        request.completeFence->giveMeResource(lock, fence);

        assert(this->device.getDevice().getFenceStatus(*fence) == vk::Result::eSuccess &&
               "Fences MUST be submitting in a signaled state to the worker");
    }

    // check if in use by threads
    {
        boost::unique_lock<boost::mutex> lock;
        vk::Fence *fence = nullptr;
        request.completeFence->giveMeResource(lock, fence);
        this->device.getDevice().resetFences(std::vector<vk::Fence>{*fence});
    }
}

void star::TransferWorker::insertRequest(std::unique_ptr<TransferManagerThread::InterThreadRequest> newRequest,
                                         const bool &isHighPriority)
{
    this->requests.push_back(std::move(newRequest));

    if (isHighPriority)
    {
        this->highPriorityRequests.push(this->requests.back().get());
    }
    else
    {
        this->standardRequests.push(this->requests.back().get());
    }
}

void star::TransferWorker::checkForCleanups()
{
    bool fullCleanupAvailable = true;
    for (int i = 0; i < this->requests.size(); i++)
    {
        if (this->requests[i])
        {
            fullCleanupAvailable = false;

            {
                boost::unique_lock<boost::mutex> lock;
                vk::Fence *fence = nullptr;
                this->requests[i]->completeFence->giveMeResource(lock, fence);

                if (this->device.getDevice().getFenceStatus(*fence) == vk::Result::eSuccess)
                {
                    this->requests[i].reset();
                }
            }
        }
    }

    if (fullCleanupAvailable)
    {
        this->requests = std::vector<std::unique_ptr<TransferManagerThread::InterThreadRequest>>();
    }
}

std::vector<std::unique_ptr<star::TransferManagerThread>> star::TransferWorker::CreateThreads(
    StarDevice &device, const std::vector<std::unique_ptr<StarQueueFamily>> &queueFamilies,
    boost::lockfree::stack<TransferManagerThread::InterThreadRequest *> &highPriorityQueue,
    boost::lockfree::stack<TransferManagerThread::InterThreadRequest *> &standardQueue)
{
    std::vector<uint32_t> allTransferQueueFamilyIndicesInUse = std::vector<uint32_t>(queueFamilies.size());
    for (int i = 0; i < queueFamilies.size(); i++)
        allTransferQueueFamilyIndicesInUse[i] = queueFamilies.at(i)->getQueueFamilyIndex();

    int curNumHighThreads = 0;
    int curNumStandardThreads = 0;
    std::vector<std::unique_ptr<TransferManagerThread>> newThreads =
        std::vector<std::unique_ptr<TransferManagerThread>>();

    for (const auto &family : queueFamilies)
    {
        int targetIndex = 0;
        for (int i = 0; i < family->getQueueCount(); i++)
        {
            std::vector<StarQueue> queues;
            queues.push_back(family->getQueues().at(targetIndex));
            targetIndex++;

            if (curNumHighThreads > curNumStandardThreads)
            {
                newThreads.emplace_back(std::make_unique<TransferManagerThread>(
                    device, device.getAllocator().get(),
                    std::vector<boost::lockfree::stack<TransferManagerThread::InterThreadRequest *> *>{&standardQueue},
                    device.getPhysicalDevice().getProperties(), queues, *family, allTransferQueueFamilyIndicesInUse));
                curNumStandardThreads++;
            }
            else
            {
                newThreads.emplace_back(std::make_unique<TransferManagerThread>(
                    device, device.getAllocator().get(),
                    std::vector<boost::lockfree::stack<TransferManagerThread::InterThreadRequest *> *>{
                        &highPriorityQueue},
                    device.getPhysicalDevice().getProperties(), queues, *family, allTransferQueueFamilyIndicesInUse));
                curNumHighThreads++;
            }
        }
    }

    return newThreads;
}