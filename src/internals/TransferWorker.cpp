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
    this->thread =
        boost::thread(TransferManagerThread::mainLoop,
                      SubThreadInfo(&this->shouldRun, this->device.getDevice(), this->familyToUse.getQueueFamilyIndex(), this->myQueues,
                                    this->device.getAllocator().get(), this->deviceProperties, &this->requestQueues,
                                    this->allTransferQueueFamilyIndicesInUse));

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
    std::queue<std::unique_ptr<ProcessRequestInfo>> processRequestInfos =
        std::queue<std::unique_ptr<ProcessRequestInfo>>(); 

    for (int i = 0; i < 5; i++)
    {
        processRequestInfos.push(CreateProcessingInfo(myInfo.device, std::make_unique<StarCommandPool>(myInfo.device, myInfo.queueFamilyIndexToUse, true)));
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

                CreateBuffer(myInfo.device, myInfo.allocator, myInfo.queues.at(0), myInfo.deviceProperties,
                             myInfo.allTransferQueueFamilyIndicesInUse, *workingInfo,
                             request->bufferTransferRequest.get(), request->resultingBuffer.value(),
                             request->gpuDoneNotificationToMain);
            }
            else if (request->textureTransferRequest)
            {
                assert(request->resultingTexture.has_value() && request->resultingTexture.value() != nullptr &&
                       "Texture request must contain both a request and a resulting address");

                CreateTexture(myInfo.device, myInfo.allocator, myInfo.queues.at(0), myInfo.deviceProperties,
                              myInfo.allTransferQueueFamilyIndicesInUse, *workingInfo,
                              request->textureTransferRequest.get(), request->resultingTexture.value(),
                              request->gpuDoneNotificationToMain);
            }

            processRequestInfos.push(std::move(workingInfo));
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
            workingInfo->markAsAvailble();
        }
    }
}

std::unique_ptr<star::TransferManagerThread::ProcessRequestInfo> star::TransferManagerThread::CreateProcessingInfo(
    vk::Device &device, std::shared_ptr<star::StarCommandPool> commandPool)
{
    return std::make_unique<ProcessRequestInfo>(
        std::make_unique<StarCommandBuffer>(device, 1, commandPool, star::Queue_Type::Ttransfer, true, false));
}

void star::TransferManagerThread::CreateBuffer(vk::Device &device, VmaAllocator &allocator, StarQueue &queue,
                                               const vk::PhysicalDeviceProperties &deviceProperties,
                                               const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
                                               ProcessRequestInfo &processInfo,
                                               TransferRequest::Buffer *newBufferRequest,
                                               std::unique_ptr<StarBuffer> *resultingBuffer,
                                               boost::atomic<bool> *gpuDoneSignalMain)
{
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

    newBufferRequest->copyFromTransferSRCToDST(*transferSrcBuffer, *resultingBuffer->get(),
                                               processInfo.commandBuffer->buffer(0));

    processInfo.commandBuffer->buffer(0).end();
    processInfo.commandBuffer->submit(0, queue.getVulkanQueue());

    processInfo.setInProcessDeps(std::move(transferSrcBuffer), gpuDoneSignalMain);
}

void star::TransferManagerThread::CreateTexture(vk::Device &device, VmaAllocator &allocator, StarQueue &queue,
                                                const vk::PhysicalDeviceProperties &deviceProperties,
                                                const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse,
                                                ProcessRequestInfo &processInfo,
                                                TransferRequest::Texture *newTextureRequest,
                                                std::unique_ptr<StarTexture> *resultingTexture,
                                                boost::atomic<bool> *gpuDoneSignalMain)
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

    newTextureRequest->copyFromTransferSRCToDST(*transferSrcBuffer, *resultingTexture->get(),
                                                processInfo.commandBuffer->buffer(0));

    processInfo.commandBuffer->buffer(0).end();
    processInfo.commandBuffer->submit(0, queue.getVulkanQueue());

    processInfo.setInProcessDeps(std::move(transferSrcBuffer), gpuDoneSignalMain);
}

void star::TransferManagerThread::CheckForCleanups(vk::Device &device,
                                                   std::queue<std::unique_ptr<ProcessRequestInfo>> &processingInfos)
{
    std::queue<std::unique_ptr<ProcessRequestInfo>> readyInfos = std::queue<std::unique_ptr<ProcessRequestInfo>>();
    std::queue<std::unique_ptr<ProcessRequestInfo>> notReadyInfos = std::queue<std::unique_ptr<ProcessRequestInfo>>();

    while (!processingInfos.empty())
    {
        std::unique_ptr<ProcessRequestInfo> workingInfo = std::move(processingInfos.front());
        processingInfos.pop();

        if (!workingInfo->isMarkedAsAvailable() && workingInfo->commandBuffer->isFenceReady(0))
        {
            workingInfo->markAsAvailble();
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

void star::TransferManagerThread::EnsureInfoReady(vk::Device &device, ProcessRequestInfo &info)
{
    if (!info.isMarkedAsAvailable())
    {
        info.commandBuffer->begin(0);
        info.markAsAvailble();
    }
    else
    {
        info.commandBuffer->begin(0);
    }
}

star::TransferManagerThread::TransferManagerThread(
    star::StarDevice &device, std::vector<boost::lockfree::stack<InterThreadRequest *> *> requestQueues,
    const vk::PhysicalDeviceProperties &deviceProperties, std::vector<StarQueue> myQueues, StarQueueFamily &familyToUse,
    const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse)
    : device(device), requestQueues(requestQueues), deviceProperties(deviceProperties), myQueues(myQueues),
      familyToUse(familyToUse), allTransferQueueFamilyIndicesInUse(allTransferQueueFamilyIndicesInUse)
{
}

void star::TransferManagerThread::cleanup()
{
    // if (this->thread.joinable())
    //     checkForCleanups(this->device.getDevice(), this->inProcessRequests);
}

star::TransferWorker::~TransferWorker()
{
    for (auto &thread : this->threads ){
        thread->stopAsync();
    }
}

star::TransferWorker::TransferWorker(star::StarDevice &device, bool overrideToSingleThreadMode)
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

void star::TransferWorker::add(boost::atomic<bool> &isBeingWorkedOnByTransferThread,
                               std::unique_ptr<TransferRequest::Buffer> newBufferRequest,
                               std::unique_ptr<star::StarBuffer> &resultingBuffer, const bool &isHighPriority)
{
    auto newRequest = std::make_unique<TransferManagerThread::InterThreadRequest>(
        &isBeingWorkedOnByTransferThread, std::move(newBufferRequest), resultingBuffer);

    insertRequest(std::move(newRequest), isHighPriority);
}

void star::TransferWorker::add(boost::atomic<bool> &isBeingWorkedOnByTransferThread,
                               std::unique_ptr<star::TransferRequest::Texture> newTextureRequest,
                               std::unique_ptr<StarTexture> &resultingTexture, const bool &isHighPriority)
{
    auto newRequest = std::make_unique<TransferManagerThread::InterThreadRequest>(
        &isBeingWorkedOnByTransferThread, std::move(newTextureRequest), resultingTexture);

    insertRequest(std::move(newRequest), isHighPriority);
}

void star::TransferWorker::update()
{
    for (auto &thread : this->threads)
    {
        thread->cleanup();
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

// void star::TransferWorker::checkForCleanups()
// {
//     bool fullCleanupAvailable = true;
//     for (int i = 0; i < this->requests.size(); i++)
//     {
//         if (this->requests[i])
//         {
//             fullCleanupAvailable = false;

//             {
//                 boost::unique_lock<boost::mutex> lock;
//                 vk::Fence *fence = nullptr;
//                 this->requests[i]->completeFence->giveMeResource(lock, fence);

//                 if (this->device.getDevice().getFenceStatus(*fence) == vk::Result::eSuccess)
//                 {
//                     this->requests[i].reset();
//                 }
//             }
//         }
//     }

//     if (fullCleanupAvailable)
//     {
//         this->requests = std::vector<std::unique_ptr<TransferManagerThread::InterThreadRequest>>();
//     }
// }

std::vector<std::unique_ptr<star::TransferManagerThread>> star::TransferWorker::CreateThreads(
    StarDevice &device, const std::vector<std::unique_ptr<StarQueueFamily>> &queueFamilies,
    boost::lockfree::stack<TransferManagerThread::InterThreadRequest *> &highPriorityQueue,
    boost::lockfree::stack<TransferManagerThread::InterThreadRequest *> &standardQueue)
{
    std::vector<uint32_t> allTransferQueueFamilyIndicesInUse = std::vector<uint32_t>(queueFamilies.size());
    for (size_t i = 0; i < queueFamilies.size(); i++)
        allTransferQueueFamilyIndicesInUse[i] = queueFamilies.at(i)->getQueueFamilyIndex();

    int curNumHighThreads = 0;
    int curNumStandardThreads = 0;
    std::vector<std::unique_ptr<TransferManagerThread>> newThreads =
        std::vector<std::unique_ptr<TransferManagerThread>>();

    for (const auto &family : queueFamilies)
    {
        int targetIndex = 0;
        for (uint32_t i = 0; i < family->getQueueCount(); i++)
        {
            std::vector<StarQueue> queues;
            queues.push_back(family->getQueues().at(targetIndex));
            targetIndex++;

            if (curNumHighThreads > curNumStandardThreads)
            {
                newThreads.emplace_back(std::make_unique<TransferManagerThread>(
                    device,
                    std::vector<boost::lockfree::stack<TransferManagerThread::InterThreadRequest *> *>{&standardQueue},
                    device.getPhysicalDevice().getProperties(), queues, *family, allTransferQueueFamilyIndicesInUse));
                curNumStandardThreads++;
            }
            else
            {
                newThreads.emplace_back(std::make_unique<TransferManagerThread>(
                    device,
                    std::vector<boost::lockfree::stack<TransferManagerThread::InterThreadRequest *> *>{
                        &highPriorityQueue},
                    device.getPhysicalDevice().getProperties(), queues, *family, allTransferQueueFamilyIndicesInUse));
                curNumHighThreads++;
            }
        }
    }

    return newThreads;
}