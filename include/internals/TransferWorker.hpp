#pragma once

#include "StarDevice.hpp"

#include <vulkan/vulkan.hpp>
#include <boost/atomic.hpp>
#include <boost/thread/thread.hpp>

#include <vector>
#include <memory>
#include <thread>

namespace star{
    class TransferThread{
        public:
            TransferThread(vk::Queue transferQueue, vk::Device vulkanDevice); 
            
            void stop() { 
                this->shouldRun.store(false);
                
                //wait for thread to exit
                this->thread.join();
            }
        private:
            vk::Queue myTransferQueue;
            vk::Device myDevice;
            boost::thread thread;
    
            boost::atomic<bool> shouldRun = false;

            static void mainLoop(vk::Queue transferQueue, vk::Device device, boost::atomic<bool>* shouldRunFlag);
    };

    class TransferWorker {
    public:
        /// @brief Creates a transfer worker which owns a DEDICATED transfer queue. Will allow asynchronous transfers
        /// @param dedicatedTransferQueueFamilyIndex The queue family index which will be ONLY used in the threads spawned by this class
        /// @param device Created star device from which vulkan objects can be made
        TransferWorker(star::StarDevice& device, std::unique_ptr<uint32_t> dedicatedTransferQueueFamilyIndex); 
        ~TransferWorker(); 

        bool isAsync() { return asyncMode; }
    private:
        std::unique_ptr<uint32_t> dedicatedTransferQueueFamilyIndex;

        std::vector<std::unique_ptr<TransferThread>> workers;

        /// Determines if this worker will run in an asynchronous mode
        const bool asyncMode = false;

        std::vector<vk::Queue> createTransferQueues(star::StarDevice& device, const uint32_t& dedicatedTransferQueueFamilyIndex); 

        void startWorkers(star::StarDevice& device, std::vector<vk::Queue> transferQueues);
    };

}