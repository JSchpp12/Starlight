#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

namespace star::core::logging
{
enum class LogLevel
{
    trace, // optional convenience
    debug,
    info,
    warning,
    error,
    fatal
};

// Map to Boost severity once, centrally.
inline constexpr boost::log::trivial::severity_level to_boost(LogLevel lvl) noexcept
{
    using B = boost::log::trivial::severity_level;
    switch (lvl)
    {
    case LogLevel::trace:
        return B::trace;
    case LogLevel::debug:
        return B::debug;
    case LogLevel::info:
        return B::info;
    case LogLevel::warning:
        return B::warning;
    case LogLevel::error:
        return B::error;
    case LogLevel::fatal:
        return B::fatal;
    default:
        return B::info;
    }
}

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

template <typename... Args> inline void log(LogLevel level, Args &&...args)
{
    log(to_boost(level), std::forward<Args>(args)...);
}

template <typename... Args> inline void error(Args &&...args)
{
    log(to_boost(LogLevel::error), std::forward<Args>(args)...);
}

template <typename... Args> inline void fatal(Args &&...args)
{
    log(to_boost(LogLevel::fatal), std::forward<Args>(args)...);
}

template <typename... Args> inline void debug(Args &&...args)
{
    log(to_boost(LogLevel::debug), std::forward<Args>(args)...);
}
template <typename... Args> inline void info(Args &&...args)
{
    log(to_boost(LogLevel::info), std::forward<Args>(args)...);
}

} // namespace star::core::logging