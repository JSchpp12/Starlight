#include "TransferWorker.hpp"

star::TransferWorker::TransferWorker(star::StarDevice& device, std::unique_ptr<uint32_t> dedicatedTransferQueueFamilyIndex) : dedicatedTransferQueueFamilyIndex(std::move(dedicatedTransferQueueFamilyIndex)), asyncMode(true) {
    //create the transfer queues
    auto transferQueues = createTransferQueues(device, *this->dedicatedTransferQueueFamilyIndex); 

    if (transferQueues.size() == 0){
        throw std::runtime_error("No Dedicated Transfer Queues Available For The Provided Queue Index");
    }

    startWorkers(device, transferQueues);
}

star::TransferWorker::~TransferWorker() {
    if (this->asyncMode){
        for (auto& worker : this->workers){
            worker->stop();
        }
    }
}

std::vector<vk::Queue> star::TransferWorker::createTransferQueues(star::StarDevice& device, const uint32_t& dedicatedTransferQueueFamilyIndex){
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

void star::TransferThread::mainLoop(vk::Queue transferQueue, vk::Device device, boost::atomic<bool>* shouldRunFlag) {
    assert(shouldRunFlag != nullptr && "Should run flag MUST be a valid object"); 
    
    std::cout << "Thread started" << std::endl;

    while(shouldRunFlag->load()){

    }

    std::cout << "Stopping thread" << std::endl;
}


void star::TransferWorker::startWorkers(star::StarDevice& device, std::vector<vk::Queue> transferQueues){
    for (int i = 0; i < transferQueues.size(); ++i){
        this->workers.emplace_back(std::make_unique<TransferThread>(transferQueues[i], device.getDevice()));
    }
}

star::TransferThread::TransferThread(vk::Queue transferQueue, vk::Device vulkanDevice) : myTransferQueue(transferQueue), myDevice(vulkanDevice){
    this->shouldRun.store(true);

    this->thread = boost::thread(star::TransferThread::mainLoop, transferQueue, vulkanDevice, &this->shouldRun);
}

