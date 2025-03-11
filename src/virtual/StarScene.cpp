#include "StarScene.hpp"

star::StarScene::StarScene(const int& numFramesInFlight) 
: myCamera(std::make_optional(std::make_unique<star::BasicCamera>(1280, 720))){

	this->lightInfoBuffers.resize(numFramesInFlight);

	for (int i = 0; i < numFramesInFlight; i++) {
		this->globalInfoBuffers.emplace_back(ManagerRenderResource::addRequest(std::make_unique<GlobalInfo>(static_cast<uint16_t>(i), *this->myCamera.value(), this->lightCounter)));
	}
}

star::StarScene::StarScene(const int& numFramesInFLight, star::StarCamera* externalCamera, std::vector<star::Handle> globalInfoBuffers)
: externalCamera(std::make_optional(externalCamera)), globalInfoBuffers(globalInfoBuffers) {

	this->lightInfoBuffers.resize(numFramesInFLight); 
}

int star::StarScene::add(std::unique_ptr<star::Light> newLight) {
	this->lightCounter++; 

	int lightIndex = this->lightList.size();
	this->lightList.emplace_back(std::move(newLight));

	//re-create light buffers
	for (int i = 0; i < this->lightInfoBuffers.size(); i++) {
		//TODO: need a function which will replace a buffer contained within the buffer manager...all handles to the information represented in that buffer need to remain valid
		if (!this->lightInfoBuffers[i].isInitialized())
			this->lightInfoBuffers[i] = ManagerRenderResource::addRequest(std::make_unique<LightInfo>(i, this->lightList));
		else
			ManagerRenderResource::updateRequest(std::make_unique<LightInfo>(i, this->lightList), this->lightInfoBuffers[i]); 
	}

	for (int i = 0; i < this->globalInfoBuffers.size(); i++){
		// if (this->myCamera.has)

		// ManagerBuffer::updateRequest(std::make_unique<GlobalInfo>(i, *this->camera, this->lightCounter), this->globalInfoBuffers[i]); 
	}
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

void star::StarScene::onWorldUpdate(const uint32_t& frameInFlightIndex){
	//check if any objects have changed since last frame, if so, they need to be updated on GPU memory

	if (this->myCamera.has_value()){
		ManagerRenderResource::updateRequest(std::make_unique<GlobalInfo>(frameInFlightIndex, *this->myCamera.value(), this->lightList.size()), this->globalInfoBuffers[frameInFlightIndex]);
	}
}