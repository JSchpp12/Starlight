#include "core/logging/LoggingFactory.hpp"

#include "starlight/common/helpers/FileHelpers.hpp"
#include "starlight/core/logging/LoggerFileBackend.hpp"

#include <star_common/helper/PathHelpers.hpp>
#include <star_common/helper/StringHelpers.hpp>

#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>

#include <chrono>
#include <filesystem>

namespace star::core::logging
{
namespace logging = boost::log;
namespace keywords = boost::log::keywords;

void init(const std::string logName)
{
    const auto baseDir = star::common::paths::GetRuntimePath().parent_path() / star::common::strings::GetStartTime();
    auto backend = boost::make_shared<LoggerFileBackend>(baseDir);
    auto sink = boost::make_shared<sinks::synchronous_sink<LoggerFileBackend>>(backend);
    sink->set_filter(logging::trivial::severity == logging::trivial::debug);
    logging::core::get()->add_sink(sink);

    auto consoleSink =
        logging::add_console_log(std::cout, keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%]: %Message%");
    consoleSink->set_filter(logging::trivial::severity != logging::trivial::debug);

    boost::log::add_common_attributes();
}

boost::log::sources::severity_logger<boost::log::trivial::severity_level> &getLoggerForThread()
{
    thread_local boost::log::sources::severity_logger<boost::log::trivial::severity_level> logger;
    return logger;
}

} // namespace star::core::logging