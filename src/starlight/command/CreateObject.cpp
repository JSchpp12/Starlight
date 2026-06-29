#include "starlight/command/CreateObject.hpp"

#include "starlight/command/detail/create_object/FromObjFileLoader.hpp"

#include <cassert>

namespace star::command
{
CreateObject::Builder &CreateObject::Builder::setLoader(std::unique_ptr<create_object::ObjectLoader> loader)
{
    m_loader = std::move(loader);
    return *this;
}

CreateObject::Builder &CreateObject::Builder::setUniqueName(std::string name)
{
    m_uniqueName = std::move(name);
    return *this;
}

CreateObject::Builder &CreateObject::Builder::setShaderResolver(ShaderResolver resolver)
{
    m_resolver = std::move(resolver);
    m_resolverSet = true;
    return *this;
}

CreateObject CreateObject::Builder::build()
{
    assert(m_loader != nullptr);
    assert(!m_uniqueName.empty());
    assert(m_resolverSet && "ShaderResolver must be provided via setShaderResolver()");

    return {std::move(m_uniqueName), std::move(m_loader), std::move(m_resolver)};
}
} // namespace star::command