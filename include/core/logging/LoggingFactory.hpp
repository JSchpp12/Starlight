#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

namespace star::core::logging
{
void init();

boost::log::sources::severity_logger<boost::log::trivial::severity_level> &getLoggerForThread();

template <typename... Args> void log(boost::log::trivial::severity_level level, Args &&...args)
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