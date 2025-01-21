#pragma once

#include "StarTexture.hpp"
#include "Handle.hpp"

#include <unordered_map>
#include <stack>
#include <functional>
#include <optional>

namespace star {
class ManagerTexture {
public:
	struct Request {
		std::optional<StarTexture::TextureCreateSettings> textureSettings;
		std::optional<std::unique_ptr<StarTexture>> createdTexture; 

		Request(StarTexture::TextureCreateSettings textureSettings) : textureSettings(textureSettings) {}
		Request(std::unique_ptr<StarTexture> createdTexture) : createdTexture(std::make_optional(std::move(createdTexture))) {}
	};

	static Handle submitRequest(std::unique_ptr<Request> newRequest); 

	//void update(); 

private:
	static size_t counter; 
	static std::stack<std::unique_ptr<Request>> newRequests; 

	std::vector<std::unique_ptr<StarTexture>> textures = std::vector<std::unique_ptr<StarTexture>>(50);
};
}