//#pragma once
//
//#include "starlight/common/entities/Light.hpp"
//#include "starlight/virtual/StarObject.hpp"
//
//#include <absl/container/flat_hash_map.h>
//#include <memory>
//#include <string>
//
//namespace star::service::scene_loader
//{
//    template <typename T>
//class SceneObjectTracker
//{
//  public:
//    SceneObjectTracker() = default;
//
//    void insert(std::pair<std::string, std::shared_ptr<StarObject>> object)
//    {
//        m_registeredObjects.insert(std::move(object));
//    }
//
//    bool contains(const std::string &uniqueName) const
//    {
//        return m_registeredObjects.contains(uniqueName) || m_registeredLights.contains(uniqueName);
//    }
//
//    const absl::flat_hash_map<std::string, std::shared_ptr<StarObject>> &getStorage() const
//    {
//        return m_registeredObjects;
//    }
//
//  private:
//    absl::flat_hash_map<std::string, std::shared_ptr<StarObject>> m_registeredObjects;
//    absl::flat_hash_map<std::string, std::shared_ptr<Light>> m_registeredLights;
//};
//} // namespace star::service::scene_loader