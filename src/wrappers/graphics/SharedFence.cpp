#include "SharedFence.hpp"

star::SharedFence::SharedFence(star::StarDevice& device, const bool& createInSignaledState) : device(device){
    vk::FenceCreateInfo info{};
    info.sType = vk::StructureType::eFenceCreateInfo;
    if (createInSignaledState)
        info.flags = vk::FenceCreateFlagBits::eSignaled;

    this->fence = device.getDevice().createFence(info);
}

star::SharedFence::~SharedFence(){
    {
        auto lock = boost::unique_lock<boost::mutex>(this->mutex);
        this->device.getDevice().destroyFence(this->fence);
    }
}

void star::SharedFence::giveMeFence(boost::unique_lock<boost::mutex>& lock, vk::Fence& fence){
    lock = boost::unique_lock<boost::mutex>(this->mutex);
    fence = this->fence;
}