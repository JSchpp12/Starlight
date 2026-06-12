#include "job/tasks/actions/WriteImageActions.hpp"

#include "starlight/core/Exceptions.hpp"

#include <algorithm>
#include <filesystem>

namespace star::job::tasks::actions
{

void ValidateExtension(const std::string &path, const std::string &expectedExt)
{
    std::filesystem::path p(path);
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext != expectedExt)
    {
        STAR_THROW("Invalid file extension: expected " + expectedExt + " but got " + ext + " for path: " + path);
    }
}

} // namespace star::job::tasks::actions