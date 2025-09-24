#pragma once

#include "Enums.hpp"

#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>

namespace star::file_helpers
{
bool FileExists(std::string_view filePath);

/// Search through directory where the provided file is located, and see if another file exists of the same name, just
/// different type/extension
std::optional<std::string> FindFileInDirectoryWithSameNameIgnoreFileType(const std::string &directoryPath,
                                                                         const std::string &name);

std::string ReadFile(const std::string &pathToFile, bool includeCarriageReturns = true);

std::string ReadFileBinary(const std::string &pathToFile); 

std::string GetFileExtension(std::string_view pathToFile);

std::string GetFileNameWithExtension(std::string_view pathToFile);

std::string GetFileNameWithoutExtension(std::string_view pathToFile);

boost::filesystem::path GetParentDirectory(const std::string &pathToFile);

Shader_Stage GetStageOfShader(std::string_view pathToFile);

void CreateDirectoryIfDoesNotExist(const boost::filesystem::path &pathToDirectory); 

} // namespace star::file_helpers