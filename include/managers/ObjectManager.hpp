//#pragma once
//
//#include "FileResourceManager.hpp"
//#include "Handle.hpp"
//#include "Vertex.hpp"
//#include "FileHelpers.hpp"
//
//#include <tiny_obj_loader.h>
//#include <glm/glm.hpp>
//
//#include <string>
//#include <vector> 
//#include <memory> 
//
//namespace star {
//    //TODO: inherit from manager base
//    class ObjectManager : public FileResourceManager<Star> {
//    public:
//        GameObject& resource(const Handle& resourceHandle) override { return this->FileResourceManager::resource(resourceHandle); }
//    protected:
//        //load the object from disk 
//        void loadObject(const std::string& pathToFile, std::vector<Vertex>* vertexList, std::vector<uint32_t>* indiciesList);
//
//        Handle createAppropriateHandle() override;
//        Handle_Type handleType() { return Handle_Type::object; }
//
//    private:
//        //TODO: replace this with the scene builder implementation
//        //GameObject* create(const std::string& pathToFile, Material* material, glm::vec3 position = glm::vec3{ 0.0f, 0.0f, 0.0f }, glm::vec3 scaleAmt = glm::vec3{ 1.0f, 1.0f, 1.0f }, Handle texture = Handle{ 0 }, Handle vertShader = Handle{ 0 }, Handle fragShader = Handle{ 1 });
//
//    };
//}
