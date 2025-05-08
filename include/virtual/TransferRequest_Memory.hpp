#pragma once

#include "StarBuffer.hpp"
#include "StarDevice.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace star::TransferRequest{
    ///Request object used by secondary thread manager to copy resources to GPU. Templated to allow for different types of creation args such as BufferCreationArgs.
    template <typename T>
    class Memory {
    public:

    Memory() = default;
    ~Memory() = default; 

    virtual std::unique_ptr<T> create(StarDevice& device); 
    
    virtual void writeData(StarBuffer& buffer) const = 0; 

    protected:

    };
}
