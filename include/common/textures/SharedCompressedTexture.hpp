#pragma once

#include "StarDevice.hpp"

#include <ktx.h>

#include <boost/thread/mutex.hpp>

#include <memory>

namespace star{
    class SharedCompressedTexture{
    public:

        SharedCompressedTexture(vk::PhysicalDevice& physicalDevice, const std::string& pathToFile); 
        
        ~SharedCompressedTexture();
        
        uint8_t getHeight(); 
        uint8_t getWidth(); 
        uint8_t getDepth(); 
        uint8_t getNumMipMaps(); 
    private:
        bool hasBeenTranscoded = false; 
    };
}