#include "helpers/FileHelpers.hpp"

#include "logging/LoggingFactory.hpp"
#include "core/Exceptions.hpp"

#include <star_common/helper/PathHelpers.hpp>

#include <sys/stat.h>
#include <iostream>
#include <sstream>

namespace star::file_helpers
{
bool FileExists(std::string_view filePath)
{
    if (boost::filesystem::exists(filePath))
    {
        if (boost::filesystem::is_regular_file(filePath))
        {
            return true;
        }
    }
    return false;
}

std::optional<std::string> FindFileInDirectoryWithSameNameIgnoreFileType(const std::string &directoryPath,
                                                                         const std::string &name)
{
    std::string found;

    boost::filesystem::path path = boost::filesystem::path(directoryPath);
    boost::filesystem::path target = boost::filesystem::path(name);

    if (boost::filesystem::exists(path) && boost::filesystem::is_directory(path))
    {

        for (auto &&x : boost::filesystem::directory_iterator(path))
        {
            boost::filesystem::path xP = boost::filesystem::path(x);

            if (xP.stem() == target.stem())
                return std::make_optional(xP.string());
        }
    }

    return std::nullopt;
}

std::string ReadFile(const std::string &pathToFile, bool includeCarriageReturns)
{
    if (!FileExists(pathToFile)){
        std::ostringstream oss; 
        oss << "Provided file for reading does not exist: " << pathToFile;
        STAR_THROW(oss.str());  
    }

    std::string line, text;
    std::ifstream fileReader(pathToFile);

    while (std::getline(fileReader, line))
    {
        text += line + (includeCarriageReturns ? "\n" : "");
    }

    return (text);
}

std::string ReadFileBinary(const std::string &pathToFile){
    if (!FileExists(pathToFile)){
        std::ostringstream oss; 
        oss << "Provided file for reading does not exist: " << pathToFile; 
        STAR_THROW(oss.str()); 
    }

    std::ifstream file(pathToFile, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::ostringstream oss; 
        oss << "Error: Could not open file " << pathToFile << std::endl;
        STAR_THROW(oss.str()); 
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(static_cast<size_t>(size));
    if (!file.read(buffer.data(), size)) {
        std::ostringstream oss; 
        oss << "Error: Could not read from file " << pathToFile << std::endl;
        core::error(oss.str()); 
        return "";
    }

    return std::string(buffer.begin(), buffer.end());
}

std::string GetFileExtension(std::string_view pathToFile)
{
    boost::filesystem::path path = boost::filesystem::path(pathToFile);

    return path.extension().string();
}

std::string GetFullPath(const std::string &pathToFile)
{
    return boost::filesystem::canonical(boost::filesystem::path(pathToFile)).string(); 
}

std::string GetFileNameWithoutExtension(const std::string &pathToFile)
{
    const auto path = boost::filesystem::path(pathToFile); 
    if (!path.has_filename()){
        STAR_THROW("No filename provided in path");  
    }

    const std::string mainFileName = path.filename().string(); 
    auto posOfExt = mainFileName.find_last_of('.');
    return mainFileName.substr(0, posOfExt);
}

std::optional<boost::filesystem::path> GetParentDirectory(const std::string &pathToFile)
{
    auto path = boost::filesystem::path(pathToFile); 
    if (!path.has_parent_path()){
        return std::nullopt;
    }else{
        return path.parent_path();
    }
}

star::Shader_Stage GetStageOfShader(std::string_view pathToFile)
{
    auto posOfExt = pathToFile.find_last_of('.');

    auto fileExt = pathToFile.substr(posOfExt);

    if (fileExt == ".vert")
    {
        return Shader_Stage::vertex;
    }
    else if (fileExt == ".frag")
    {
        return Shader_Stage::fragment;
    }

    STAR_THROW("Unsupported stage type for shader"); 
}

void CreateDirectoryIfDoesNotExist(const boost::filesystem::path &pathToDirectory){
    try
    {
        boost::filesystem::create_directories(pathToDirectory);
    }
    catch (const boost::filesystem::filesystem_error &e)
    {
        std::ostringstream oss;
        oss << "Filesystem error: " << e.what();

        STAR_THROW(oss.str()); 
    }
}

boost::filesystem::path GetExecutableDirectory(){
    const std::string exePath = star::common::GetRuntimePath().string();
    const auto parentDir = GetParentDirectory(exePath).value(); //should always have a value
    return parentDir.string(); 
}
} // namespace star::file_helpers
