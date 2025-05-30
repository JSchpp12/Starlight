#pragma once

#include "StarDevice.hpp"

#include "ThreadSharedResource.hpp"

#include <vulkan/vulkan.hpp>

namespace star{
    class SharedFence : public ThreadSharedResource<vk::Fence>{
        public:
        SharedFence();
        SharedFence(StarDevice& device, const bool& createInSignaledState);
        virtual ~SharedFence() override; 

        bool operator==(const SharedFence& other) const{
            return this->resource == other.resource;
        }

        bool operator!=(const SharedFence& other) const{
            return this->resource != other.resource;
        }

        bool operator!() const{
            return !this->resource;
        }

        protected:
        StarDevice& device; 

        static vk::Fence CreateFence(StarDevice& device, const bool& createInSignaledState); 
    };
}