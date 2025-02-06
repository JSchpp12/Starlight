#pragma once

#include "ManagerPlug.hpp"
#include "ManagerTexture.hpp"
#include "StarImage.hpp"

#include <functional>
#include <memory>

namespace star {
class TextureModifier : public ManagerPlug {
public:
	TextureModifier(std::unique_ptr<StarImage> texture);

	TextureModifier(StarImage::TextureCreateSettings settings); 

protected:
	void init(); 

	void update(); 

private: 

};
}