#pragma once 

#include "Enums.hpp"

#include <sys/stat.h>

#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <optional>

namespace star {
struct FileHelpers {
    static bool FileExists(const std::string& filePath);

    ///Search through directory where the provided file is located, and see if another file exists of the same name, just different type/extension
    static std::optional<std::string> FindFileInDirectoryWithSameNameIgnoreFileType(const std::string& directoryPath, const std::string& name); 

    static std::string ReadFile(std::string pathToFile, bool includeCarriageReturns = true) {
        std::string line, text;
        std::ifstream fileReader(pathToFile);

        while (std::getline(fileReader, line)) {
            text += line + (includeCarriageReturns ? "\n" : "");
        }

        return(text);
    }

    static std::string GetFileExtension(const std::string& pathToFile);

    static std::string GetFileNameWithExtension(const std::string& pathToFile) {
        return pathToFile.substr(pathToFile.find_last_of("/\\") + 1);
    }

    static std::string GetFileNameWithoutExtension(const std::string& pathToFile) {
        const std::string mainFileName = GetFileNameWithExtension(pathToFile);
        auto posOfExt = mainFileName.find_last_of('.');
        return mainFileName.substr(0, posOfExt);
    }

    static std::string GetBaseFileDirectory(const std::string& pathToFile) {
        size_t found = pathToFile.find_last_of("/\\");
        return pathToFile.substr(0, found) + "/";
    }

    //Get file type of provided file -- shaders
    static Shader_File_Type GetFileType(const std::string& pathToFile) {
        auto posOfExt = pathToFile.find_last_of('.');

        auto fileExtension = pathToFile.substr(posOfExt);

        if (fileExtension == ".spv") {
            return Shader_File_Type::spirv;
        }
        else if (fileExtension == ".frag" || fileExtension == ".vert") {
            return Shader_File_Type::glsl;
        }

        throw std::runtime_error("Unsupported file type provided");
    }

    static Shader_Stage GetStageOfShader(const std::string& pathToFile) {
        auto posOfExt = pathToFile.find_last_of('.');

        auto fileExt = pathToFile.substr(posOfExt);

        if (fileExt == ".vert") {
            return Shader_Stage::vertex;
        }
        else if (fileExt == ".frag") {
            return Shader_Stage::fragment;
        }

        throw std::runtime_error("Unsupported stage type for shader");
    }
};
}