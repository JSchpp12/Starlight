#pragma once

#include "ManagerPlug.hpp"
#include "ManagerTexture.hpp"
#include "StarTexture.hpp"

#include <functional>
#include <memory>

namespace star {
class TextureModifier : public ManagerPlug {
public:
	TextureModifier(std::unique_ptr<StarTexture> texture);

	TextureModifier(StarTexture::TextureCreateSettings settings); 

protected:
	void init(); 

	void update(); 

private: 

};
}