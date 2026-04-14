#pragma once

#include "starlight/service/detail/scene_loader/IEntityReader.hpp"
#include "starlight/virtual/StarObject.hpp"

#include <nlohmann/json.hpp>

namespace star::service::scene_loader
{
class ObjectLoader : public IEntityReader
{
  public:
    virtual bool canLoad(const nlohmann::json &objectDatas, const std::string &uniqueName) const noexcept override;

    void load(const nlohmann::json &objectDatas, const std::string &uniqueName, StarObject &obj) const noexcept;

  private:
    struct LoadedObjectInfo
    {
        glm::vec3 position;
        glm::vec3 scale;
        std::array<std::pair<star::Type::Axis, float>, 3> rotationsToApply;
    };
};
} // namespace star::service::scene_loader