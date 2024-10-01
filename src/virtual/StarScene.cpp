#include "StarScene.hpp"

int star::StarScene::add(std::unique_ptr<star::Light> newLight) {
	int lightIndex = this->lightList.size();
	this->lightList.emplace_back(std::move(newLight));
	return lightIndex;
}

int star::StarScene::add(std::unique_ptr<StarObject> newObject) {
	int objHandle = objCounter;
	this->objects.insert(std::pair<int, std::unique_ptr<StarObject>>(objCounter, std::move(newObject)));
	this->objCounter++;
	return objHandle;
}

std::vector<std::reference_wrapper<star::StarObject>> star::StarScene::getObjects()
{
	auto result = std::vector<std::reference_wrapper<StarObject>>();
	for (auto& obj : this->objects) {
		result.push_back(*obj.second);
	}
	return result; 
}
