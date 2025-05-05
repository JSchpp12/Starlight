#include "SharedFence.hpp"

star::SharedFence::SharedFence(star::StarDevice& device, const bool& createInSignaledState) 
: device(device), star::ThreadSharedResource<vk::Fence>(new vk::Fence(createFence(device, createInSignaledState))){

}

vk::Fence star::SharedFence::createFence(StarDevice& device, const bool& createInSignaledState){
    vk::FenceCreateInfo info{};
    info.sType = vk::StructureType::eFenceCreateInfo;
    if (createInSignaledState)
        info.flags = vk::FenceCreateFlagBits::eSignaled;

    return device.getDevice().createFence(info);
}

void star::SharedFence::destroyResource(){
    auto lock = boost::unique_lock<boost::mutex>(this->mutex);
    this->device.getDevice().destroyFence(*this->resource);
}