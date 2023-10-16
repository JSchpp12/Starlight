#pragma once 

#include "Color.hpp"

#include <stb_image.h>

#include <memory> 
#include <vector> 
#include <string> 
#include <stdexcept>
#include <optional>
#include <iostream>
#include <optional>

namespace star {

class Texture {
public:
    int width=0, height=0, channels=0;
    std::string pathToFile = "";
    bool onDisk=false;

    Texture(int texWidth, int texHeight)
        : rawData(std::make_optional<std::vector<std::vector<Color>>>(std::vector<std::vector<Color>>(texWidth, std::vector<Color>(texHeight, Color{ 0,0,0,0 } )))),
        width(texWidth), height(texHeight), channels(4) {};

    Texture(std::vector<std::vector<Color>> rawData, int texWidth, int texHeight)
        : rawData(rawData), width(texWidth),
        height(texHeight), channels(3){
        onDisk = false;
    }
    /// <summary>
    /// Initalize texture with provided raw data. For use with programatically generated textures. 
    /// </summary>
    /// <param name="rawTextureData"></param>
    /// <param name="texWidth"></param>
    /// <param name="texHeight"></param>
    /// <param name="texChannels"></param>
    Texture(const std::string& pathToImage)
        : pathToFile(pathToImage) {
        onDisk = true;
        init();
    }

    void init() {
        //load from disk to get properties of image
        auto pixelData = stbi_load(pathToFile.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!pixelData) {
            throw std::runtime_error("Unable to load image");
        }
        stbi_image_free(pixelData);
    }

    /// <summary>
    /// Read the image from disk into memory or provide the image which is in memory
    /// </summary>
    /// <returns></returns>
    std::unique_ptr<unsigned char> data() {
        //load from disk
        if (onDisk) {
            std::unique_ptr<unsigned char> pixelData(stbi_load(pathToFile.c_str(), &width, &height, &channels, STBI_rgb_alpha));
            if (!pixelData) {
                throw std::runtime_error("Unable to load image");
            }

            return std::move(pixelData);
        }
        else if (rawData.has_value()) {
            
            //std::copy(rawData->begin(), rawData->end(), data.get());
            //return std::move(data);
            int numPix = this->height * this->width * this->channels; 
            int pixCounter = 0; 

            auto data = std::vector<unsigned char>(numPix);
            for (int i = 0; i < height; i++) {
                for (int j = 0; j < width; j++) {
                    data.at(channels * pixCounter + 0) = rawData.value().at(i).at(j).raw_r();
                    data.at(channels * pixCounter + 1) = rawData.value().at(i).at(j).raw_g();
                    data.at(channels * pixCounter + 2) = rawData.value().at(i).at(j).raw_b();
                    data.at(channels * pixCounter + 3) = rawData.value().at(i).at(j).raw_a();
                    pixCounter++;
                }
            }

            auto prepData = std::unique_ptr<unsigned char>(new unsigned char[data.size()]);
            std::copy(data.begin(), data.end(), prepData.get());

            return std::move(prepData); 

        }
        else {
            return std::unique_ptr<unsigned char>(new unsigned char[] {0x00, 0x00, 0x00, 0x00});
        }
    }

    /// <summary>
    /// Get a pointer to the raw storage data of an in-memory texture
    /// </summary>
    /// <returns></returns>
    std::vector<std::vector<Color>>* getRawData() {
        if (!rawData.has_value())
            return nullptr;
        return &rawData.value();
    }

protected:
    std::optional<std::vector<std::vector<Color>>> rawData;

};

}