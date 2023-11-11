#pragma once 

#include "StarTexture.hpp"

namespace star {
	/// <summary>
	/// Texture which only exists on the gpu. Will not be created or readable from host. 
	/// </summary>
	class ComputeTexture : public StarTexture {
	public:
		ComputeTexture(int width, int height); 

		// Inherited via StarTexture
		std::unique_ptr<unsigned char> data() override;
		int getWidth() override;
		int getHeight() override;
		int getChannels() override;

	private:
		int height, width; 

	};
}