#include "starlight/core/logging/LoggerFileBackend.hpp"

#include "starlight/common/helpers/FileHelpers.hpp"

#include <boost/date_time/posix_time/posix_time.hpp>

#include <thread>

namespace star::core::logging
{
namespace expr = boost::log::expressions;

void LoggerFileBackend::consume(logging::record_view const &rec)
{
    auto &stream = getStream();
    if (!stream.is_open())
        return;

    stream << "[" << logging::extract<boost::posix_time::ptime>("TimeStamp", rec) << "] "
           << "[" << logging::extract<logging::attributes::current_thread_id::value_type>("ThreadID", rec) << "] "
           << "[" << logging::extract<logging::trivial::severity_level>("Severity", rec) << "] "
           << ": " << rec[expr::smessage] << "\n";
    stream.flush();
}

std::ofstream &LoggerFileBackend::getStream()
{
    thread_local std::ofstream file = openFile();
    return file;
}

std::ofstream LoggerFileBackend::openFile()
{
    std::ostringstream name;
    name << "thread_" << std::this_thread::get_id() << ".log";

    const auto path = m_baseDir / name.str();
    star::file_helpers::CreateDirectoryIfDoesNotExist(path.parent_path());

    return std::ofstream(path.string(), std::ios::app);
}
} // namespace star::core::logging