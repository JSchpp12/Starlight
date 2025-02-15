#pragma once
#include "Handle.hpp"
#include "StarObject.hpp"
#include "Light.hpp"
#include "StarWindow.hpp"
#include "StarRenderObject.hpp"
#include "GlobalInfo.hpp"
#include "BasicCamera.hpp"
#include "LightInfo.hpp"

#include <map>
#include <memory>
#include <vector>

namespace star {
	/// <summary>
	/// Container for all objects in a scene. 
	/// </summary>
	class StarScene {
	public:
		StarScene(const int& numFramesInFlight) : camera(std::make_shared<BasicCamera>(1280, 720)){
			for (int i = 0; i < numFramesInFlight; i++) {
				this->globalInfoBuffers.emplace_back(std::make_shared<GlobalInfo>(static_cast<uint16_t>(i), *this->camera, this->lightCounter)); 
				this->lightInfoBuffers.emplace_back(std::make_shared<LightInfo>(static_cast<uint16_t>(i), this->lightList));
			}
		};


		StarScene(const int& numFramesInFLight, std::shared_ptr<StarCamera> camera, std::vector<std::shared_ptr<GlobalInfo>> globalInfoBuffers)
			: camera(camera), globalInfoBuffers(globalInfoBuffers) {
			for (int i = 0; i < numFramesInFLight; i++) {
				this->lightInfoBuffers.emplace_back(std::make_shared<LightInfo>(static_cast<uint16_t>(i), this->lightList)); 
			}
		};

		virtual ~StarScene() = default;

		int add(std::unique_ptr<Light> newLight);

		int add(std::unique_ptr<StarObject> newObject);

		std::shared_ptr<StarCamera> getCamera() const { return this->camera; }

		StarObject& getObject(int objHandle) {
			return *this->objects.at(objHandle);
		}

		Light& getLight(int light) {
			return *this->lightList.at(light); 
		}

		std::vector<std::unique_ptr<Light>>& getLights() { return this->lightList; }
		std::vector<std::reference_wrapper<StarObject>> getObjects(); 
		std::shared_ptr<GlobalInfo> getGlobalInfoBuffer(const int& index) {return this->globalInfoBuffers.at(index); }
		std::shared_ptr<LightInfo> getLightInfoBuffer(const int& index) { return this->lightInfoBuffers.at(index); }
	protected:
		std::vector<std::shared_ptr<GlobalInfo>> globalInfoBuffers = std::vector<std::shared_ptr<GlobalInfo>>(); 
		std::vector<std::shared_ptr<LightInfo>> lightInfoBuffers = std::vector<std::shared_ptr<LightInfo>>();

		int objCounter = 0; 
		int rObjCounter = 0; 
		int lightCounter = 0; 
		std::shared_ptr<StarCamera> camera = nullptr; 

		std::unordered_map<int, std::unique_ptr<StarObject>> objects = std::unordered_map<int, std::unique_ptr<StarObject>>(); 
		std::vector<std::unique_ptr<Light>> lightList = std::vector<std::unique_ptr<Light>>();
	};
}