#pragma once

#include "starlight/command/detail/create_object/ObjectLoader.hpp"

#include <string>

namespace star::command::create_object
{
class FromObjFileLoader : public ObjectLoader
{
    public:
    explicit FromObjFileLoader(std::string objFilePath);
    virtual ~FromObjFileLoader() = default;

    virtual std::shared_ptr<StarObject> load() override; 

    private:
    std::string m_objFilePath; 
};
} // namespace star::command::create_object