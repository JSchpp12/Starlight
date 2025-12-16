#include "core/logging/LoggingFactory.hpp"

namespace star::core::logging
{
void init()
{
    boost::log::add_console_log(std::cout,
                                boost::log::keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%]: %Message%");
    boost::log::add_common_attributes();
}

boost::log::sources::severity_logger<boost::log::trivial::severity_level> &getLoggerForThread()
{
    thread_local boost::log::sources::severity_logger<boost::log::trivial::severity_level> logger;
    return logger;
}

} // namespace star::core::logging