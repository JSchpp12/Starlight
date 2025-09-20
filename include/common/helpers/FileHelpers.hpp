#pragma once

#include "Enums.hpp"

#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace star::file_helpers
{
bool FileExists(std::string_view filePath);

/// Search through directory where the provided file is located, and see if another file exists of the same name, just
/// different type/extension
std::optional<std::string> FindFileInDirectoryWithSameNameIgnoreFileType(const std::string &directoryPath,
                                                                         const std::string &name);

std::string ReadFile(std::string pathToFile, bool includeCarriageReturns = true);

std::string GetFileExtension(std::string_view pathToFile);

std::string GetFileNameWithExtension(std::string_view pathToFile);

std::string GetFileNameWithoutExtension(std::string_view pathToFile);

std::string GetParentDirectory(std::string_view pathToFile);

Shader_Stage GetStageOfShader(std::string_view pathToFile);

void CreateDirectoryIfDoesNotExist(std::string_view pathToDirectory);

} // namespace star::file_helpers