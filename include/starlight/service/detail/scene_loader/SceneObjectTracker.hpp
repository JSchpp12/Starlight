#pragma once

#include "starlight/virtual/StarObject.hpp"

#include <absl/container/flat_hash_map.h>
#include <memory>
#include <string>


namespace star::service::scene_loader
{
class SceneObjectTracker
{
  public:
    SceneObjectTracker() = default;

    void insert(std::pair<std::string, std::shared_ptr<StarObject>> object)
    {
        m_storage.insert(std::move(object));
    }

    bool contains(const std::string &uniqueName) const
    {
        return m_storage.contains(uniqueName);
    }

    const absl::flat_hash_map<std::string, std::shared_ptr<StarObject>> &getStorage() const
    {
        return m_storage;
    }

  private:
    absl::flat_hash_map<std::string, std::shared_ptr<StarObject>> m_storage;
};
} // namespace star::service::scene_loader