#include "starlight/common/io/JSONFileWriter.hpp"

#include "starlight/core/Exceptions.hpp"

#include <iostream>
#include <fstream>

namespace star::common::io
{
int JSONFileWriter::operator()(const std::string &path)
{
    // Ensure directory exists
    try
    {
        std::filesystem::path p(path);
        if (p.has_parent_path())
        {
            std::filesystem::create_directories(p.parent_path());
        }
    }
    catch (...)
    {
        // Non-fatal; try writing anyway
    }

    // Pretty print with 2-space indentation
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs)
    {
        STAR_THROW(std::string("Failed to open JSON for writing: ") + path);
    }
    ofs << std::setw(2) << m_jData;
    ofs.flush();
    if (!ofs)
    {
        STAR_THROW(std::string("Failed to write JSON to: ") + path);
    }

    return 0;
}
}