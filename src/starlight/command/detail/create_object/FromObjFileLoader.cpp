#include "starlight/command/detail/create_object/FromObjFileLoader.hpp"

#include "starlight/object/BasicObject.hpp"

namespace star::command::create_object
{
FromObjFileLoader::FromObjFileLoader(std::string objFilePath) : m_objFilePath(std::move(objFilePath))
{
}

std::shared_ptr<StarObject> FromObjFileLoader::load(core::device::DeviceContext &context,
                                                    ShaderResolver &shaderResolver)
{
    auto materialProviders = BasicObject::MakeMaterialProviders(context, m_objFilePath);
    return std::make_shared<BasicObject>(std::move(m_objFilePath), std::move(materialProviders), shaderResolver);
}

} // namespace star::command::create_object