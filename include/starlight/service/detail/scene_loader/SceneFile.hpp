#pragma once

#include "starlight/enums/Enums.hpp"
#include "starlight/service/detail/scene_loader/SceneObjectTracker.hpp"

#include <absl/container/flat_hash_map.h>
#include <glm/glm.hpp>
#include <string>
#include <array>
#include <optional>

namespace star::service::scene_loader
{
class SceneFile
{
  public:
    struct LoadedObjectInfo
    {
        glm::vec3 position;
        glm::vec3 scale;
        std::array<std::pair<star::Type::Axis, float>, 3> rotationsToApply;
    };

    explicit SceneFile(std::string path) : m_path(std::move(path)){};

    void write(const SceneObjectTracker &sceneObjects);

    std::optional<LoadedObjectInfo> tryReadObjectInfo(const std::string &name);

  private:
    std::string m_path;
};
} // namespace star::service::scene_loader