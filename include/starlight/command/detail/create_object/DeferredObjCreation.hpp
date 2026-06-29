#pragma once

#include "starlight/ShaderResolver.hpp"
#include "starlight/command/detail/create_object/ObjectLoader.hpp"

#include <functional>
#include <memory>

namespace star::command::create_object
{
class DeferredObjCreation : public ObjectLoader
{
  public:
    using Factory = std::function<std::shared_ptr<StarObject>(ShaderResolver &)>;

    explicit DeferredObjCreation(Factory factory) : m_factory(std::move(factory))
    {
    }

    virtual ~DeferredObjCreation() = default;

    virtual std::shared_ptr<StarObject> load(ShaderResolver &shaderResolver) override
    {
        return m_factory(shaderResolver);
    }

  private:
    Factory m_factory;
};
} // namespace star::command::create_object