#include "ObjectManager.hpp"

star::StarObject& star::ObjectManager::resource(const Handle& resourceHandle)
{
	return *this->objects.at(resourceHandle.id); 
}

star::Handle star::ObjectManager::add(std::unique_ptr<StarObject> newObject)
{
	Handle newHandle; 
	this->objects.push_back(std::move(newObject)); 

	newHandle.id = this->objects.size() - 1; 
	newHandle.type = star::Handle_Type::object; 
	return newHandle; 
}
