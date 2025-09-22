#include "FileHelpers.hpp"

#include <boost/filesystem.hpp>

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
        throw std::runtime_error(oss.str()); 
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
        throw std::runtime_error(oss.str()); 
    }

    std::ifstream file(pathToFile, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << pathToFile << std::endl;
        return "";
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        std::cerr << "Error: Could not read from file " << pathToFile << std::endl;
        return "";
    }

    return std::string(buffer.begin(), buffer.end());
}

std::string GetFileExtension(std::string_view pathToFile)
{
    boost::filesystem::path path = boost::filesystem::path(pathToFile);

    return path.extension().string();
}

std::string GetFileNameWithExtension(std::string_view pathToFile)
{
    return std::string(pathToFile.substr(pathToFile.find_last_of("/\\") + 1));
}

std::string GetFileNameWithoutExtension(std::string_view pathToFile)
{
    const std::string mainFileName = GetFileNameWithExtension(pathToFile);
    auto posOfExt = mainFileName.find_last_of('.');
    return mainFileName.substr(0, posOfExt);
}

std::string GetParentDirectory(const std::string &pathToFile, const bool appendDirectorySeparator)
{
    auto path = boost::filesystem::path(pathToFile); 
    if (!boost::filesystem::path(pathToFile).has_parent_path()){
        std::ostringstream oss; 
        oss << "Requested path does not have a parent. Path: " << pathToFile; 
        throw std::runtime_error(oss.str());
    }

    if (appendDirectorySeparator){
        return boost::filesystem::canonical(path.parent_path()).string() + boost::filesystem::path::separator;
    }else{
        return boost::filesystem::canonical(path.parent_path()).string(); 
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

    throw std::runtime_error("Unsupported stage type for shader");
}

void CreateDirectoryIfDoesNotExist(const std::string &pathToDirectory)
{
    boost::filesystem::path targetDir(pathToDirectory);

    try
    {
        boost::filesystem::create_directories(targetDir);
    }
    catch (const boost::filesystem::filesystem_error &e)
    {
        std::ostringstream oss;
        oss << "Filesystem error: " << e.what();

        throw std::runtime_error(oss.str());
    }
}
} // namespace star::file_helpers
