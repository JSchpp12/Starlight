#include "GameObject.hpp"

void star::GameObject::prepRender(StarDevice& device) {
	for (auto& mesh : meshes) {
		mesh->getMaterial().prepRender(device); 
	}
}