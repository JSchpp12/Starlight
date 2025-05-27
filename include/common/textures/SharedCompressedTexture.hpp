#pragma once

#include "StarDevice.hpp"
#include "ThreadSharedResource.hpp"

#include <ktx.h>

#include <boost/thread/mutex.hpp>

#include <memory>

namespace star{
    class SharedCompressedTexture : private ThreadSharedResource<ktxTexture2>{
    public:
        SharedCompressedTexture(const std::string& pathToFile, const vk::PhysicalDevice& physicalDevice); 
        
        ~SharedCompressedTexture() override;

        void giveMeTranscodedImage(boost::unique_lock<boost::mutex>& lock, ktxTexture2*& texture); 
        
        uint8_t getHeight() const; 
        uint8_t getWidth() const; 
        uint8_t getDepth() const; 
        uint8_t getNumMipMaps() const; 
        std::string getPathToFile(){return this->pathToFile;}

    private:
        const std::string pathToFile; 
        bool hasBeenTranscoded = false; 
        ktx_transcode_fmt_e selectedTranscodeTargetFormat; 

        void createKTextureFromFile(); 

        static void GetSupportedCompressedTextureFormats(const vk::PhysicalDevice& physicalDevice, 
            std::vector<ktx_transcode_fmt_e>& availableFormats, std::vector<std::string>& availableFormatNames);

        static bool IsFormatSupported(const vk::PhysicalDevice& physicalDevice, 
            const vk::Format& format); 

        static ktx_transcode_fmt_e SelectTranscodeFormat(const std::vector<ktx_transcode_fmt_e>& availableFormats, const std::vector<std::string>& availableFormatNames);

        static void VerifyFiles(const std::string& imagePath); 

        void loadKTX();

        void transcode(); 
    };
}