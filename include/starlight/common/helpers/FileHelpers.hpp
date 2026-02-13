#pragma once

#include "Enums.hpp"

#include <boost/filesystem.hpp>

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
std::vector<boost::filesystem::path> FindFilesInDirectoryWithSameNameIgnoreFileType(const std::string &directoryPath,
                                                                                    const std::string &name);

std::string ReadFile(const std::string &pathToFile, bool includeCarriageReturns = true);

std::string ReadFileBinary(const std::string &pathToFile);

std::string GetFileExtension(std::string_view pathToFile);

std::string GetFullPath(const std::string &pathToFile);

std::string GetFileNameWithoutExtension(const std::string &pathToFile);

std::optional<boost::filesystem::path> GetParentDirectory(const std::string &pathToFile);

Shader_Stage GetStageOfShader(std::string_view pathToFile);

void CreateDirectoryIfDoesNotExist(const boost::filesystem::path &pathToDirectory);

boost::filesystem::path GetExecutableDirectory();
} // namespace star::file_helpers