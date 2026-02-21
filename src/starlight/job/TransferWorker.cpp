#include "job/TransferWorker.hpp"

#include "core/Exceptions.hpp"
#include "logging/LoggingFactory.hpp"
#include <star_common/helper/CastHelpers.hpp>

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
                                                                           this->deviceProperties, m_taskContainer,
                                                                           this->allTransferQueueFamilyIndicesInUse));

    if (!this->thread.joinable())
        throw std::runtime_error("Failed to launch transfer thread");
}

void star::job::TransferManagerThread::stopAsync()
{
    this->shouldRun.store(false);

    core::logging::log(boost::log::trivial::info, "Waiting for transfer thread to exit");

    // wait for thread to exit
    this->thread.join();
}

void star::job::TransferManagerThread::mainLoop(job::TransferManagerThread::SubThreadInfo myInfo)
{
    core::logging::log(boost::log::trivial::info, "Transfer thread started");

    std::queue<std::unique_ptr<ProcessRequestInfo>> processRequestInfos =
        std::queue<std::unique_ptr<ProcessRequestInfo>>();
    auto pool = std::make_shared<StarCommandPool>(myInfo.device, myInfo.queue.getParentQueueFamilyIndex(), true);

    for (int i = 0; i < 1; i++)
    {
        processRequestInfos.push(std::make_unique<ProcessRequestInfo>(
            pool, std::make_unique<StarCommandBuffer>(myInfo.device, 1, pool.get(), star::Queue_Type::Ttransfer, true,
                                                      false)));
    }

    while (myInfo.shouldRun->load())
    {
        InterThreadRequest *request = nullptr;
        bool allEmpty = true;

        // try to get a request
        if (auto result = myInfo.taskContainer->getQueuedTask())
        {
            request = &result.value();
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

                core::logging::log(boost::log::trivial::info, "Creating Texture");

                CreateTexture(myInfo.device, myInfo.allocator, myInfo.queue, request->gpuWorkDoneSemaphore,
                              myInfo.deviceProperties, myInfo.allTransferQueueFamilyIndicesInUse, *workingInfo,
                              request->textureTransferRequest.get(), request->resultingTexture.value(),
                              request->gpuDoneNotificationToMain);
            }

            processRequestInfos.push(std::move(workingInfo));

            request->gpuDoneNotificationToMain->store(true);
            request->gpuDoneNotificationToMain->notify_all();
        }
        else
        {
            CheckForCleanups(myInfo.device, processRequestInfos);

            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }

    core::logging::log(boost::log::trivial::info, "Transfer thread exiting");

    while (!processRequestInfos.empty())
    {
        std::unique_ptr<ProcessRequestInfo> workingInfo = std::move(processRequestInfos.front());
        processRequestInfos.pop();

        if (!workingInfo->isMarkedAsAvailable())
        {
            workingInfo->commandBuffer->wait(0);
            workingInfo->markAsAvailable(myInfo.device);
        }
    }

    CheckForCleanups(myInfo.device, processRequestInfos);

    // cleanup command pools
    pool->cleanupRender(myInfo.device);
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
    processInfo.commandBuffer->submit(0, queue.getVulkanQueue(), nullptr, nullptr, nullptr, &signalSemaphore);

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
    processInfo.commandBuffer->submit(0, queue.getVulkanQueue(), nullptr, nullptr, nullptr, &signalSemaphores);

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
            workingInfo->markAsAvailable(device);
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
        info.markAsAvailable(device);
    }
    else
    {
        info.commandBuffer->begin(0);
    }
}

star::job::TransferManagerThread::TransferManagerThread(
    std::shared_ptr<job::TaskContainer<TransferManagerThread::InterThreadRequest, 1000>> taskContainer,
    const vk::PhysicalDeviceProperties &deviceProperties, StarQueue myQueue,
    const std::vector<uint32_t> &allTransferQueueFamilyIndicesInUse)
    : m_taskContainer(taskContainer), deviceProperties(deviceProperties), myQueue(myQueue),
      allTransferQueueFamilyIndicesInUse(allTransferQueueFamilyIndicesInUse)
{
}

star::job::TransferWorker::~TransferWorker()
{
    stopAll();
}

star::job::TransferWorker::TransferWorker(star::core::device::StarDevice &device, bool overrideToSingleThreadMode,
                                          std::vector<StarQueue *> &queuesToUse)
{
    this->threads = CreateThreads(device, queuesToUse, m_highPriorityTaskContainer, m_standardTaskContainer);

    if (this->threads.size() == 0)
    {
        STAR_THROW("Failed to create any transfer worker");
    }

    for (auto &thread : this->threads)
        thread->startAsync(device);
}

void star::job::TransferWorker::add(boost::atomic<bool> &isBeingWorkedOnByTransferThread,
                                    vk::Semaphore signalWhenDoneSemaphore,
                                    std::unique_ptr<TransferRequest::Buffer> newBufferRequest,
                                    std::unique_ptr<star::StarBuffers::Buffer> &resultingBuffer,
                                    const bool &isHighPriority)
{
    auto newRequest = std::make_unique<job::TransferManagerThread::InterThreadRequest>;

    insertRequest({&isBeingWorkedOnByTransferThread, std::move(signalWhenDoneSemaphore), std::move(newBufferRequest),
                   resultingBuffer},
                  isHighPriority);
}

void star::job::TransferWorker::add(boost::atomic<bool> &isBeingWorkedOnByTransferThread,
                                    vk::Semaphore signalWhenDoneSemaphore,
                                    std::unique_ptr<star::TransferRequest::Texture> newTextureRequest,
                                    std::unique_ptr<StarTextures::Texture> &resultingTexture,
                                    const bool &isHighPriority)
{
    insertRequest(job::TransferManagerThread::InterThreadRequest(&isBeingWorkedOnByTransferThread,
                                                                 std::move(signalWhenDoneSemaphore),
                                                                 std::move(newTextureRequest), resultingTexture),
                  isHighPriority);
}

void star::job::TransferWorker::update()
{
    // for (auto &thread : this->threads)
    // {
    //     // thread->cleanup();
    // }
}

void star::job::TransferWorker::insertRequest(job::TransferManagerThread::InterThreadRequest newRequest,
                                              const bool &isHighPriority)
{
    if (isHighPriority)
    {
        m_highPriorityTaskContainer->queueTask(std::move(newRequest));
    }
    else
    {
        m_standardTaskContainer->queueTask(std::move(newRequest));
    }
}

std::vector<std::unique_ptr<star::job::TransferManagerThread>> star::job::TransferWorker::CreateThreads(
    core::device::StarDevice &device, const std::vector<StarQueue *> &queuesToUse,
    std::shared_ptr<job::TaskContainer<TransferManagerThread::InterThreadRequest, 1000>> highPriorityQueue,
    std::shared_ptr<job::TaskContainer<TransferManagerThread::InterThreadRequest, 1000>> standardQueue)
{
    std::vector<uint32_t> allTransferQueueFamilyIndicesInUse = std::vector<uint32_t>();

    std::set<uint32_t> uniqueQueueFamilyIndicesInUse = std::set<uint32_t>();
    for (const auto &queue : queuesToUse)
    {
        uniqueQueueFamilyIndicesInUse.insert(queue->getParentQueueFamilyIndex());
    }

    for (const auto &index : uniqueQueueFamilyIndicesInUse)
    {
        allTransferQueueFamilyIndicesInUse.push_back(index);
    }

    std::set<size_t> alreadyInUseIndex = std::set<size_t>();

    std::vector<std::unique_ptr<job::TransferManagerThread>> newThreads =
        std::vector<std::unique_ptr<job::TransferManagerThread>>();

    bool createHighPriority = true;
    for (const auto *queue : queuesToUse)
    {
        if (createHighPriority)
        {
            newThreads.emplace_back(std::make_unique<job::TransferManagerThread>(
                highPriorityQueue, device.getPhysicalDevice().getProperties(), *queue,
                allTransferQueueFamilyIndicesInUse));
            createHighPriority = false;
        }
        else
        {
            newThreads.emplace_back(std::make_unique<job::TransferManagerThread>(
                standardQueue, device.getPhysicalDevice().getProperties(), *queue, allTransferQueueFamilyIndicesInUse));
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