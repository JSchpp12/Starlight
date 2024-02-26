#pragma once 

#include "Color.hpp"

#include "StarTexture.hpp"

#include <stb_image.h>

#include <memory> 
#include <vector> 
#include <string> 
#include <stdexcept>
#include <optional>
#include <iostream>
#include <optional>

namespace star {

    class Texture : public StarTexture {
    public:
        std::string pathToFile = "";
        bool onDisk = false;

        Texture(int texWidth, int texHeight)
            : width(texWidth), height(texHeight), channels(4), onDisk(false) {};

        Texture(int texWidth, int texHeight, TextureCreateSettings& settings)
            : StarTexture(settings), rawData(std::make_optional<std::vector<std::vector<Color>>>(std::vector<std::vector<Color>>(texWidth, std::vector<Color>(texHeight, Color{})))),
            width(texWidth), height(texHeight), channels(4), onDisk(false) {};

        Texture(std::vector<std::vector<Color>> rawData)
            : rawData(rawData), width(rawData.size()),
            height(rawData.front().size()), channels(4) {};

        Texture(int texWidth, int texHeight, std::vector<std::vector<Color>> rawData)
            : rawData(rawData), width(texWidth),
            height(texHeight), channels(4), onDisk(false) {};

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
            loadFromDisk();
        }

        int getHeight() override {return this->height; }
        int getWidth() override { return this->width; }
        int getChannels() override { return this->channels; }

        void loadFromDisk() {
            //load from disk to get properties of image
            auto pixelData = stbi_load(pathToFile.c_str(), &width, &height, &channels, STBI_rgb_alpha);
            if (!pixelData) {
                throw std::runtime_error("Unable to load image");
            }
            stbi_image_free(pixelData);
        }


        virtual std::unique_ptr<unsigned char> data() override {
            //load from disk
            if (onDisk) {
                std::unique_ptr<unsigned char> pixelData(stbi_load(pathToFile.c_str(), &width, &height, &channels, STBI_rgb_alpha));

                //apply overriden alpha value if needed
                if (this->overrideAlpha.has_value() && channels == 4) {
                    for (int i = 0; i < height; i++) {
                        for (int j = 0; j < width; j++) {
                            unsigned char* pixelOffset = pixelData.get() + (i + height * j) * channels; 
                            pixelOffset[3] = this->overrideAlpha.value().raw_a(); 
                        }
                    }
                }

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
                        if (this->channels == 3) {
                            data.at(channels * pixCounter + 0) = rawData.value().at(i).at(j).raw_r();
                            data.at(channels * pixCounter + 1) = rawData.value().at(i).at(j).raw_g();
                            data.at(channels * pixCounter + 2) = rawData.value().at(i).at(j).raw_b();
                        } else if (this->channels == 4) {
                            data.at(channels * pixCounter + 0) = rawData.value().at(i).at(j).raw_r();
                            data.at(channels * pixCounter + 1) = rawData.value().at(i).at(j).raw_g();
                            data.at(channels * pixCounter + 2) = rawData.value().at(i).at(j).raw_b();
                            if (this->overrideAlpha.has_value())
                                data.at(channels * pixCounter + 3) = this->overrideAlpha.value().raw_a(); 
                            else
                                data.at(channels * pixCounter + 3) = rawData.value().at(i).at(j).raw_a();
                        }
                        pixCounter++;
                    }
                }

                auto prepData = std::unique_ptr<unsigned char>(new unsigned char[data.size()]);
                std::copy(data.begin(), data.end(), prepData.get());

                return std::move(prepData);

            }
            else {
                return std::unique_ptr<unsigned char>();
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

        void setAlpha(Color setAlpha) {
            this->overrideAlpha = setAlpha; 
        }

    protected:
        std::optional<Color> overrideAlpha; 
        std::optional<std::vector<std::vector<Color>>> rawData;
        int width = 0, height = 0, channels = 0;
    };

}