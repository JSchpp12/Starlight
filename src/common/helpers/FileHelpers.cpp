#include "FileHelpers.hpp"

#include <boost/filesystem.hpp>

bool star::FileHelpers::FileExists(const std::string& pathToFile){
    if (boost::filesystem::exists(pathToFile)){
        if (boost::filesystem::is_regular_file(pathToFile)){
            return true;
        }
    }
    return false; 
}

std::optional<std::string> star::FileHelpers::FindFileInDirectoryWithSameNameIgnoreFileType(const std::string& directoryPath, const std::string& name){
    std::string found;
    
    boost::filesystem::path path = boost::filesystem::path(directoryPath); 
    boost::filesystem::path target = boost::filesystem::path(name);

    if (boost::filesystem::exists(path) && boost::filesystem::is_directory(path)){

        for (auto&& x : boost::filesystem::directory_iterator(path)){
            boost::filesystem::path xP = boost::filesystem::path(x); 

            if (xP.stem() == target.stem())
                return std::make_optional(xP.string());
        }
    }

    return std::nullopt;
}

std::string star::FileHelpers::GetFileExtension(const std::string& pathToFile){
    boost::filesystem::path path = boost::filesystem::path(pathToFile); 

    return path.extension().string();
}