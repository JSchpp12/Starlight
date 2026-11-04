#pragma once

#include <star_common/Handle.hpp>

#include <string>

namespace star::core::device
{
class DeviceContext;
}

namespace star::maker::texture
{
/// @brief Loads a skybox description from a JSON file and submits the texture transfer request that backs it.
class MakeSkybox
{
  public:
    explicit MakeSkybox(core::device::DeviceContext &deviceContext);

    /// @brief Name of the skybox texture file. Must be set before calling make(). The sidecar info file is
    ///        resolved from this name as <name-without-extension>.skyboxinfo.json.
    MakeSkybox &setTexture(std::string texture);

    /// @brief Queues the info read, submits the transfer request, and returns the handle to the resulting texture.
    [[nodiscard]] Handle make();

  private:
    core::device::DeviceContext &m_deviceContext;
    std::string m_texture;
};
} // namespace star::maker::texture
