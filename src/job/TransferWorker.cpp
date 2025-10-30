#include "job/TransferWorker.hpp"

#include "CastHelpers.hpp"

#include <syncstream>
#include <thread>

star::job::TransferManagerThread::~TransferManagerThread()
{
    if (this->thread.joinable())
    {
        this->stopAsync();
    }
}

void star::job::TransferManagerThread::startAsync(core::device::StarDevice &device)
{
    this->shouldRun.store(true);
    this->thread = boost::thread(job::TransferManagerThread::mainLoop,
                                 job::TransferManagerThread::SubThreadInfo(&this->shouldRun, device.getVulkanDevice(),
                                                                           this->myQueue, device.getAllocator().get(),
                                                                           this->deviceProperties, &this->requestQueues,
                                                                           this->allTransferQueueFamilyIndicesInUse));

    if (!this->thread.joinable())
        throw std::runtime_error("Failed to launch transfer thread");
}

void star::job::TransferManagerThread::stopAsync()
{
    this->shouldRun.store(false);

    std::cout << "Waiting for thread to exit" << std::endl;

    // wait for thread to exit
    this->thread.join();
}

void star::job::TransferManagerThread::mainLoop(job::TransferManagerThread::SubThreadInfo myInfo)
{
    std::cout << "Transfer thread started..." << std::endl;
    std::queue<std::unique_ptr<ProcessRequestInfo>> processRequestInfos =
        std::queue<std::unique_ptr<ProcessRequestInfo>>();

    for (int i = 0; i < 5; i++)
    {
        processRequestInfos.push(CreateProcessingInfo(
            myInfo.device,
            std::make_unique<StarCommandPool>(myInfo.device, myInfo.queue.getParentQueueFamilyIndex(), true)));
    }

    while (myInfo.shouldRun->load())
    {
        InterThreadRequest *request = nullptr;
        bool allEmpty = true;

        // try to get a request
        for (size_t i = 0; i < myInfo.workingRequestQueues->size(); i++)
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
            CheckForCleanups(myInfo.device, processRequestInfos);

            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        else
        {
            std::unique_ptr<ProcessRequestInfo> workingInfo = std::move(processRequestInfos.front());
            processRequestInfos.pop();

            EnsureInfoReady(myInfo.device, *workingInfo);

            if (request->bufferTransferRequest)
            {
                assert(request->resultingBuffer.has_value() && request->resultingBuffer.value() != nullptr &&
                       "Buffer request must contain both a request and a resulting address");

                request->bufferTransferRequest->prep();

                CreateBuffer(myInfo.device, myInfo.allocator, myInfo.queue, request->gpuWorkDoneSemaphore,
                             myInfo.deviceProperties, myInfo.allTransferQueueFamilyIndicesInUse, *workingInfo,
                             request->bufferTransferRequest.get(), request->resultingBuffer.value(),
                             request->gpuDoneNotificationToMain);
            }
            else if (request->textureTransferRequest)
            {
                assert(request->resultingTexture.has_value() && request->resultingTexture.value() != nullptr &&
                       "Texture request must contain both a request and a resulting address");

                request->textureTransferRequest->prep();

                CreateTexture(myInfo.device, myInfo.allocator, myInfo.queue, request->gpuWorkDoneSemaphore,
                              myInfo.deviceProperties, myInfo.allTransferQueueFamilyIndicesInUse, *workingInfo,
                              request->textureTransferRequest.get(), request->resultingTexture.value(),
                              request->gpuDoneNotificationToMain);
            }

            processRequestInfos.push(std::move(workingInfo));

            request->gpuDoneNotificationToMain->store(true);
            request->gpuDoneNotificationToMain->notify_all();
        }
    }

    std::cout << "Transfer Thread exiting..." << std::endl;

    while (!processRequestInfos.empty())
    {
        std::unique_ptr<ProcessRequestInfo> workingInfo = std::move(processRequestInfos.front());
        processRequestInfos.pop();

        if (!workingInfo->isMarkedAsAvailable())
        {
            workingInfo->commandBuffer->wait(0);
            workingInfo->markAsAvailable();
        }
    }

    CheckForCleanups(myInfo.device, processRequestInfos);
}

std::unique_ptr<star::job::TransferManagerThread::ProcessRequestInfo> star::job::TransferManagerThread::
    CreateProcessingInfo(vk::Device &device, std::shared_ptr<star::StarCommandPool> commandPool)
{
    return std::make_unique<ProcessRequestInfo>(
        std::make_unique<StarCommandBuffer>(device, 1, commandPool, star::Queue_Type::Ttransfer, true, false));
}

void star::job::TransferManagerThread::CreateBuffer(vk::Device &device, VmaAllocator &allocator, StarQueue &queue,
                                                    vk::Semaphore signalWhenDoneSemaphore,
                                                    const vk::PhysicalDeviceProperties &deviceProperties,
                                                    const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
                                                    ProcessRequestInfo &processInfo,
                                                    TransferRequest::Buffer *newBufferRequest,
                                                    std::unique_ptr<StarBuffers::Buffer> *resultingBuffer,
                                                    boost::atomic<bool> *gpuDoneSignalMain)
{
    auto transferSrcBuffer = newBufferRequest->createStagingBuffer(device, allocator);
    if (transferSrcBuffer->getBufferSize() == 0)
    {
        throw std::runtime_error("Failed to create transfer src buffer");
    }

    {
        auto newResult = newBufferRequest->createFinal(device, allocator, allTransferQueueFamilyIndicesInUse);
        if (newResult->getBufferSize() == 0)
        {
            throw std::runtime_error("Failed to create final buffer");
        }
        if (!*resultingBuffer ||
            (resultingBuffer->get() && newResult->getBufferSize() > resultingBuffer->get()->getBufferSize()))
        {
            resultingBuffer->swap(newResult);
        }
    }

    newBufferRequest->writeDataToStageBuffer(*transferSrcBuffer);

    newBufferRequest->copyFromTransferSRCToDST(*transferSrcBuffer, *resultingBuffer->get(),
                                               processInfo.commandBuffer->buffer(0));

    processInfo.commandBuffer->buffer(0).end();

    auto signalSemaphore = std::vector<vk::Semaphore>{signalWhenDoneSemaphore};
    processInfo.commandBuffer->submit(0, queue.getVulkanQueue(), nullptr, nullptr, &signalSemaphore);

    processInfo.setInProcessDeps(std::move(transferSrcBuffer));
}

void star::job::TransferManagerThread::CreateTexture(vk::Device &device, VmaAllocator &allocator, StarQueue &queue,
                                                     vk::Semaphore signalWhenDoneSemaphore,
                                                     const vk::PhysicalDeviceProperties &deviceProperties,
                                                     const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
                                                     ProcessRequestInfo &processInfo,
                                                     TransferRequest::Texture *newTextureRequest,
                                                     std::unique_ptr<StarTextures::Texture> *resultingTexture,
                                                     boost::atomic<bool> *gpuDoneSignalMain)
{

    auto transferSrcBuffer = newTextureRequest->createStagingBuffer(device, allocator);

    // should eventually implement option to just re-use existing image
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

    newTextureRequest->copyFromTransferSRCToDST(*transferSrcBuffer, *resultingTexture->get(),
                                                processInfo.commandBuffer->buffer(0));

    processInfo.commandBuffer->buffer(0).end();
    auto signalSemaphores = std::vector<vk::Semaphore>{signalWhenDoneSemaphore};
    processInfo.commandBuffer->submit(0, queue.getVulkanQueue(), nullptr, nullptr, &signalSemaphores);

    processInfo.setInProcessDeps(std::move(transferSrcBuffer));
}

void star::job::TransferManagerThread::CheckForCleanups(
    vk::Device &device, std::queue<std::unique_ptr<ProcessRequestInfo>> &processingInfos)
{
    std::queue<std::unique_ptr<ProcessRequestInfo>> readyInfos = std::queue<std::unique_ptr<ProcessRequestInfo>>();
    std::queue<std::unique_ptr<ProcessRequestInfo>> notReadyInfos = std::queue<std::unique_ptr<ProcessRequestInfo>>();

    while (!processingInfos.empty())
    {
        std::unique_ptr<ProcessRequestInfo> workingInfo = std::move(processingInfos.front());
        processingInfos.pop();

        if (!workingInfo->isMarkedAsAvailable() && workingInfo->commandBuffer->isFenceReady(0))
        {
            workingInfo->markAsAvailable();
            readyInfos.push(std::move(workingInfo));
        }
        else
        {
            notReadyInfos.push(std::move(workingInfo));
        }
    }

    while (!readyInfos.empty())
    {
        processingInfos.push(std::move(readyInfos.front()));
        readyInfos.pop();
    }

    while (!notReadyInfos.empty())
    {
        processingInfos.push(std::move(notReadyInfos.front()));
        notReadyInfos.pop();
    }
}

void star::job::TransferManagerThread::EnsureInfoReady(vk::Device &device, ProcessRequestInfo &info)
{
    if (!info.isMarkedAsAvailable())
    {
        info.commandBuffer->begin(0);
        info.markAsAvailable();
    }
    else
    {
        info.commandBuffer->begin(0);
    }
}

star::job::TransferManagerThread::TransferManagerThread(
    std::vector<boost::lockfree::stack<star::job::TransferManagerThread::InterThreadRequest *> *> requestQueues,
    const vk::PhysicalDeviceProperties &deviceProperties, StarQueue myQueue,
    const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse)
    : requestQueues(requestQueues), deviceProperties(deviceProperties), myQueue(myQueue),
      allTransferQueueFamilyIndicesInUse(allTransferQueueFamilyIndicesInUse)
{
}

star::job::TransferWorker::~TransferWorker()
{
    stopAll();
}

star::job::TransferWorker::TransferWorker(star::core::device::StarDevice &device, bool overrideToSingleThreadMode,
                                          std::vector<StarQueue> &queuesToUse)
{
    this->threads = CreateThreads(device, queuesToUse, this->highPriorityRequests, this->standardRequests);

    if (this->threads.size() == 0)
        throw std::runtime_error("Failed to create transfer worker");

    for (auto &thread : this->threads)
        thread->startAsync(device);
}

void star::job::TransferWorker::add(boost::atomic<bool> &isBeingWorkedOnByTransferThread,
                                    vk::Semaphore signalWhenDoneSemaphore,
                                    std::unique_ptr<TransferRequest::Buffer> newBufferRequest,
                                    std::unique_ptr<star::StarBuffers::Buffer> &resultingBuffer,
                                    const bool &isHighPriority)
{
    auto newRequest = std::make_unique<job::TransferManagerThread::InterThreadRequest>(
        &isBeingWorkedOnByTransferThread, std::move(signalWhenDoneSemaphore), std::move(newBufferRequest),
        resultingBuffer);

    insertRequest(std::move(newRequest), isHighPriority);
}

void star::job::TransferWorker::add(boost::atomic<bool> &isBeingWorkedOnByTransferThread,
                                    vk::Semaphore signalWhenDoneSemaphore,
                                    std::unique_ptr<star::TransferRequest::Texture> newTextureRequest,
                                    std::unique_ptr<StarTextures::Texture> &resultingTexture,
                                    const bool &isHighPriority)
{
    auto newRequest = std::make_unique<job::TransferManagerThread::InterThreadRequest>(
        &isBeingWorkedOnByTransferThread, std::move(signalWhenDoneSemaphore), std::move(newTextureRequest),
        resultingTexture);

    insertRequest(std::move(newRequest), isHighPriority);
}

void star::job::TransferWorker::update()
{
    // for (auto &thread : this->threads)
    // {
    //     // thread->cleanup();
    // }
}

void star::job::TransferWorker::insertRequest(
    std::unique_ptr<job::TransferManagerThread::InterThreadRequest> newRequest, const bool &isHighPriority)
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

std::vector<std::unique_ptr<star::job::TransferManagerThread>> star::job::TransferWorker::CreateThreads(
    core::device::StarDevice &device, const std::vector<StarQueue> queuesToUse,
    boost::lockfree::stack<job::TransferManagerThread::InterThreadRequest *> &highPriorityQueue,
    boost::lockfree::stack<job::TransferManagerThread::InterThreadRequest *> &standardQueue)
{
    std::vector<uint32_t> allTransferQueueFamilyIndicesInUse = std::vector<uint32_t>();

    std::set<uint32_t> uniqueQueueFamilyIndicesInUse = std::set<uint32_t>();
    for (const auto &queue : queuesToUse)
    {
        uniqueQueueFamilyIndicesInUse.insert(queue.getParentQueueFamilyIndex());
    }

    for (const auto &index : uniqueQueueFamilyIndicesInUse)
    {
        allTransferQueueFamilyIndicesInUse.push_back(index);
    }

    std::set<size_t> alreadyInUseIndex = std::set<size_t>();

    int curNumHighThreads = 0;
    int curNumStandardThreads = 0;
    std::vector<std::unique_ptr<job::TransferManagerThread>> newThreads =
        std::vector<std::unique_ptr<job::TransferManagerThread>>();

    for (const auto &queue : queuesToUse)
    {
        if (curNumHighThreads > curNumStandardThreads)
        {
            newThreads.emplace_back(std::make_unique<job::TransferManagerThread>(
                std::vector<boost::lockfree::stack<job::TransferManagerThread::InterThreadRequest *> *>{&standardQueue},
                device.getPhysicalDevice().getProperties(), queue, allTransferQueueFamilyIndicesInUse));

            curNumStandardThreads++;
        }
        else
        {
            newThreads.emplace_back(std::make_unique<job::TransferManagerThread>(
                std::vector<boost::lockfree::stack<job::TransferManagerThread::InterThreadRequest *> *>{
                    &highPriorityQueue},
                device.getPhysicalDevice().getProperties(), queue, allTransferQueueFamilyIndicesInUse));
            curNumHighThreads++;
        }
    }

    return newThreads;
}

void star::job::TransferWorker::stopAll()
{
    for (auto &thread : this->threads)
    {
        thread->stopAsync();
    }
}