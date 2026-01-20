#pragma once

#include "starlight/service/detail/scene_loader/SceneObjectTracker.hpp"

#include <string>

namespace star::service::scene_loader
{
class SceneFile
{
  public:
    explicit SceneFile(std::string path) : m_path(std::move(path)){};

    void write(const SceneObjectTracker &sceneObjects);

    SceneObjectTracker read();

  private:
    std::string m_path;
};
} // namespace star::service::scene_loader