#include "Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION   
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

star::Texture::Texture(int texWidth, int texHeight)
	: width(texWidth), height(texHeight), channels(4), onDisk(false) {}

star::Texture::Texture(int texWidth, int texHeight, vk::SubresourceLayout texLayout, unsigned char* raw)
	: width(texWidth), height(texHeight), channels(3), onDisk(false) {
	this->rawData = std::make_optional<std::vector<std::vector<Color>>>(std::vector<std::vector<Color>>(texHeight, std::vector<Color>(texWidth, Color{})));
    for (int i = 0; i < texHeight; i++) {
        unsigned int* row = (unsigned int*)raw;

		for (int j = 0; j < texWidth; j++) {
            unsigned char r = *(unsigned char*)(row+2);
            unsigned char g = *(unsigned char*)(row + 1);
            unsigned char b = *(unsigned char*)(row);
            //unsigned char ra = raw[this->channels * (j + texLayout.rowPitch * i) + 0];
            //unsigned char ga = raw[this->channels * (j + texLayout.rowPitch * i) + 1];
            //unsigned char ba = raw[this->channels * (j + texLayout.rowPitch * i) + 2];

            if (r != 0 || g != 0 || b != 0) {
                std::cout << "col test" << std::endl;
            }            

			this->rawData.value().at(i).at(j) = Color(r, g, b);

            row++;
		}
        raw += texLayout.rowPitch;
	}
}
star::Texture::Texture(std::vector<std::vector<Color>> rawData) 
	: rawData(rawData), width(rawData.size()), 
	height(rawData.front().size()), channels(4) {}

star::Texture::Texture(int texWidth, int texHeight, std::vector<std::vector<Color>> rawData) 
	: rawData(rawData), width(texWidth),
	height(texHeight), channels(4), onDisk(false) {}

star::Texture::Texture(const std::string& pathToImage)
	: pathToFile(pathToImage) {
	onDisk = true;
	loadFromDisk();
}

void star::Texture::loadFromDisk()
{
	//load from disk to get properties of image
	auto pixelData = stbi_load(pathToFile.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	if (!pixelData) {
		throw std::runtime_error("Unable to load image");
	}
	stbi_image_free(pixelData);
}

std::unique_ptr<unsigned char> star::Texture::data()
{
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
                }
                else if (this->channels == 4) {
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

void star::Texture::saveToDisk(const std::string& path)
{
    if (this->rawData) {
        std::unique_ptr<unsigned char> data = this->data(); 

        stbi_write_png(path.c_str(), this->width, this->height, this->channels, data.get(), this->width * this->channels);
    }
}
