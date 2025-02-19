#ifndef STAR_MEMORY_TRANSFER_REQUEST_HPP
#define STAR_MEMORY_TRANSFER_REQUEST_HPP

#include "StarDevice.hpp"
#include "Allocator.hpp"

#include <boost/atomic.hpp>
#include <vulkan/vulkan.hpp>

#include <functional>
#include <memory>

namespace star{
    class MemoryTransferRequest {
    public:
        MemoryTransferRequest(boost::atomic<vk::Fence> const * transferCompleteFence)
        : transferCompleteFence(transferCompleteFence){
        }

        //needs to be fully containerized! Give all data into this object to allow for load of buffers with data
        MemoryTransferRequest(const MemoryTransferRequest& other) = default;

        
    protected:
        
    private:
        //fence not owned by this object, will have to be destroyed elsewhere
        boost::atomic<vk::Fence> const *transferCompleteFence;
    };
}

#endif