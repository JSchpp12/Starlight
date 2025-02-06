#pragma once 

#include "Color.hpp"

#include "StarImage.hpp"

#include <vulkan/vulkan.hpp>
#include <memory> 
#include <vector> 
#include <string> 
#include <stdexcept>
#include <optional>
#include <iostream>

namespace star {

    class FileTexture : public StarImage {
    public:
        std::string pathToFile = "";
        bool onDisk = false;

        /// <summary>
        /// Initalize texture with provided raw data. For use with programatically generated textures. 
        /// </summary>
        /// <param name="rawTextureData"></param>
        /// <param name="texWidth"></param>
        /// <param name="texHeight"></param>
        /// <param name="texChannels"></param>
        FileTexture(const std::string& pathToImage);

        void setAlpha(Color setAlpha) {
            this->overrideAlpha = setAlpha; 
        }

        void saveToDisk(const std::string& path);

    protected:
        std::optional<Color> overrideAlpha; 

        /// @brief Create a staging buffer with the prepared texture data loaded
        /// @param device Reference to the device to be used when creating the buffer.
        /// @return 
        std::unique_ptr<StarBuffer> loadImageData(StarDevice& device) override; 

        static int getTextureHeight(const std::string& pathToFile);

        static int getTextureWidth(const std::string& pathToFile);

        static int getTextureChannels(const std::string& pathToFile);
    };
}