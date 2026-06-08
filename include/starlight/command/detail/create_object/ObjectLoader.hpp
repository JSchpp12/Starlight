#pragma once

#include "starlight/object/StarObject.hpp"

#include <memory>

namespace star::command::create_object
{
class ObjectLoader
{
  public:
    virtual ~ObjectLoader() = default;
    virtual std::shared_ptr<StarObject> load() = 0;
};
} // namespace star::command::create_object