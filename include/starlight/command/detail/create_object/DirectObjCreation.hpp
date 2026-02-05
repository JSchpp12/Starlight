#pragma once

#include "starlight/command/detail/create_object/ObjectLoader.hpp"

namespace star::command::create_object
{
class DirectObjCreation : public ObjectLoader
{
  public:
    explicit DirectObjCreation(std::shared_ptr<StarObject> object);
    virtual ~DirectObjCreation() = default;

    virtual std::shared_ptr<StarObject> load() override
    {
        return m_object;
    }

  private:
    std::shared_ptr<StarObject> m_object;
};
} // namespace star::command::create_object