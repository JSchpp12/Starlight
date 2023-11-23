#include "ComputeTexture.hpp"

star::ComputeTexture::ComputeTexture(int width, int height) : width(width), height(height)
{
}

std::unique_ptr<unsigned char> star::ComputeTexture::data()
{
	return std::unique_ptr<unsigned char>();
}

int star::ComputeTexture::getWidth()
{
	return width; 
}

int star::ComputeTexture::getHeight()
{
	return height;
}

int star::ComputeTexture::getChannels()
{
	return 3;
}
