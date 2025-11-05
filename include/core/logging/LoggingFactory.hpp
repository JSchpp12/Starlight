#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

namespace star::core::logging
{
static void init(){
    boost::log::add_console_log(std::cout,
                                boost::log::keywords::format = "[%TimeStamp%] [%ThreadID%] [%Severity%]: %Message%");
    boost::log::add_common_attributes();
}

static boost::log::sources::severity_logger<boost::log::trivial::severity_level>& getLoggerForThread(){
    thread_local boost::log::sources::severity_logger<boost::log::trivial::severity_level> logger;
    return logger;
}

template <typename... Args> static void log(boost::log::trivial::severity_level level, Args &&...args)
{
    auto &logger = getLoggerForThread();
    boost::log::record rec = logger.open_record(boost::log::keywords::severity = level);
    if (rec)
    {
        boost::log::record_ostream strm(rec);
        (strm << ... << args); // Fold expression: expands to strm << arg1 << arg2 << ...
        strm.flush();
        logger.push_record(std::move(rec));
    }
}
} // namespace star::core::logging