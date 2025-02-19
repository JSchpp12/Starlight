#pragma once

#include "ManagerBuffer.hpp"
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
		StarScene(const int& numFramesInFlight);

		StarScene(const int& numFramesInFlight, std::shared_ptr<StarCamera> camera, std::vector<Handle> globalInfoBuffers);

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
		Handle getGlobalInfoBuffer(const int& index) {return this->globalInfoBuffers.at(index); }
		Handle getLightInfoBuffer(const int& index) { return this->lightInfoBuffers.at(index); }
	protected:
		std::vector<Handle> globalInfoBuffers = std::vector<Handle>(); 
		std::vector<Handle> lightInfoBuffers = std::vector<Handle>();
		Handle cameraInfo = Handle(); 

		int objCounter = 0; 
		int rObjCounter = 0; 
		int lightCounter = 0; 
		std::shared_ptr<StarCamera> camera = nullptr; 

		std::unordered_map<int, std::unique_ptr<StarObject>> objects = std::unordered_map<int, std::unique_ptr<StarObject>>(); 
		std::vector<std::unique_ptr<Light>> lightList = std::vector<std::unique_ptr<Light>>();
	};
}