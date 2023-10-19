#pragma once
#include "Handle.hpp"
#include "StarObject.hpp"
#include "Light.hpp"
#include "StarWindow.hpp"

#include "StarCamera.hpp"

#include <map>
#include <memory>

namespace star {
	/// <summary>
	/// Container for all objects in a scene. 
	/// </summary>
	class StarScene {
	public:
		StarScene() = default;
		virtual ~StarScene() = default;

		virtual void add(std::unique_ptr<Light> newLight) { this->lightList.emplace_back(std::move(newLight));}
		virtual int add(std::unique_ptr<StarObject> newObject) { 
			int objHandle = objCounter; 
			this->objectList.insert(std::pair<int, std::unique_ptr<StarObject>>(objCounter, std::move(newObject))); 
			objCounter++; 
			return objHandle; 
		}

		StarObject& getObject(int objHandle) {
			return *this->objectList.at(objHandle);
		}

		virtual std::vector<std::unique_ptr<Light>>& getLights() { return this->lightList; }
		virtual std::unordered_map<int, std::unique_ptr<StarObject>>& getObjects() { return this->objectList; }
	protected:
		int objCounter = 0; 
		//std::vector<std::unique_ptr<StarObject>> objectList;
		std::unordered_map<int, std::unique_ptr<StarObject>> objectList; 
		std::vector<std::unique_ptr<Light>> lightList;
		

	};
}