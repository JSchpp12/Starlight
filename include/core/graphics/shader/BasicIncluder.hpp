#pragma once

#include <memory>
#include <shaderc/shaderc.hpp>
#include <string>

namespace star::core::graphics::shader
{
/// @brief Notifies shaderC to include the parent of the requesting file along with any additional provided paths
class BasicIncluder : public shaderc::CompileOptions::IncluderInterface
{
  public:
    BasicIncluder(std::vector<std::string> additionalIncludePaths)
        : m_additionalIncludePaths(std::move(additionalIncludePaths))
    {
    }
    virtual ~BasicIncluder() = default;

    shaderc_include_result *GetInclude(const char *requestedSource, shaderc_include_type type,
                                       const char *requestingSource, size_t includeDepth) override;

    void ReleaseInclude(shaderc_include_result *data) override;

  private:
    struct Payload
    {
        std::string name;
        std::string data;
    };

    std::vector<std::string> m_additionalIncludePaths;
    std::unique_ptr<Payload> m_payload = nullptr;
    std::unique_ptr<std::string> m_errorMessage = nullptr; 
    std::unique_ptr<shaderc_include_result> m_result = nullptr;
};
} // namespace star::core::graphics::shader