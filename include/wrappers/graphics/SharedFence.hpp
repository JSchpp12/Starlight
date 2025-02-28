#pragma once

#include "StarDevice.hpp"

#include <vulkan/vulkan.hpp>
#include <boost/thread/mutex.hpp>

namespace star{
    class SharedFence{
        public:
        SharedFence();
        SharedFence(StarDevice& device, const bool& createInSignaledState);
        ~SharedFence();

        bool operator==(const SharedFence& other) const{
            return this->fence == other.fence;
        }

        bool operator!=(const SharedFence& other) const{
            return this->fence != other.fence;
        }

        bool operator!() const{
            return !this->fence;
        }

        void giveMeFence(boost::unique_lock<boost::mutex>& lock, vk::Fence& fence);

        protected:
        StarDevice& device; 

        vk::Fence fence = vk::Fence();
        boost::mutex mutex = boost::mutex();
    };
}