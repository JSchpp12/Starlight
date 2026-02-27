#include "graphics/shader/BasicIncluder.hpp"

#include "FileHelpers.hpp"

#include <fstream>
#include <vector>

namespace star::core::graphics::shader
{
shaderc_include_result *BasicIncluder::GetInclude(const char *requestedSource, shaderc_include_type type,
                                                  const char *requestingSource, size_t includeDepth)
{
    std::vector<std::filesystem::path> searchPaths;

    if (type == shaderc_include_type_relative && requestedSource && *requestingSource)
    {
        std::filesystem::path path = std::filesystem::path(requestedSource); 
        if (path.has_parent_path())
        {
            searchPaths.emplace_back(path.parent_path());
        }
    }

    for (auto &inc : m_additionalIncludePaths)
    {
        searchPaths.emplace_back(std::filesystem::path(inc));
    }

    std::filesystem::path resolved;
    for (auto &path : searchPaths)
    {
        boost::system::error_code error;

        auto candidate = path / requestedSource;
        if (std::filesystem::exists(candidate, error))
        {
            resolved = std::filesystem::canonical(candidate);
            if (!error)
            {
                break;
            }
        }
    }

    if (resolved.empty())
    {
        m_result = std::make_unique<shaderc_include_result>(); 
        m_errorMessage = std::make_unique<std::string>("Could not find include for: " + std::string(requestedSource));
        m_result->content = m_errorMessage->c_str();
        m_result->content_length = m_errorMessage->size();
        return m_result.get();
    }

    // might want to check for cycle includes here later

    std::ifstream file(resolved.string(), std::ios::binary);
    std::string contents = file_helpers::ReadFileBinary(resolved.string()); 

    m_payload = std::unique_ptr<Payload>{new Payload{.name = resolved.string(), .data = std::move(contents)}};

    m_result = std::make_unique<shaderc_include_result>(); 
    m_result->source_name = m_payload->name.c_str(); 
    m_result->source_name_length = m_payload->name.size();
    m_result->content = m_payload->data.c_str(); 
    m_result->content_length = m_payload->data.size(); 
    m_result->user_data = m_payload.get(); 
    return m_result.get(); 
}

void BasicIncluder::ReleaseInclude(shaderc_include_result *data)
{
    m_result.release(); 
    m_errorMessage.release();
    m_result.release();  
}
} // namespace star::core::graphics::shader