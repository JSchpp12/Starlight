#include "starlight/command/detail/create_object/FromObjFileLoader.hpp"

#include "starlight/common/objects/BasicObject.hpp"

namespace star::command::create_object
{
FromObjFileLoader::FromObjFileLoader(std::string objFilePath) : m_objFilePath(std::move(objFilePath))
{
}

std::shared_ptr<StarObject> FromObjFileLoader::load()
{
    return std::make_shared<BasicObject>(std::move(m_objFilePath));
}

} // namespace star::command::create_object