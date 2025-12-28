
#pragma once
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#if __has_include(<source_location>) && __cplusplus >= 202002L
#include <source_location>
#define STAR_HAVE_SOURCE_LOCATION 1
#else
#define STAR_HAVE_SOURCE_LOCATION 0
#endif

#include "core/logging/LoggingFactory.hpp"

namespace star::core
{

using namespace star::core::logging;

// If you prefer a named engine exception, alias or define your own here.
struct RuntimeError : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

// Helper to build a single string from multiple parts
template <typename... Args> inline std::string make_message(std::string_view base, Args &&...args)
{
    std::ostringstream oss;
    oss << base;
    (oss << ... << std::forward<Args>(args));
    return std::move(oss).str();
}

// ---------------------------
// Log & throw (no Vulkan)
// ---------------------------
#if STAR_HAVE_SOURCE_LOCATION
[[noreturn]] inline void throw_error(std::string_view msg, std::source_location loc = std::source_location::current())
{
    using namespace star::core::logging;
    // Log with rich context
    error("[", loc.file_name(), ":", loc.line(), " ", loc.function_name(), "] ", msg);
    // Throw standard runtime_error
    throw RuntimeError(std::string(msg));
}

template <typename... Args>
[[noreturn]] inline void throw_error_fmt(std::source_location loc, std::string_view base, Args &&...args)
{
    const auto full = make_message(base, std::forward<Args>(args)...);
    throw_error(full, loc);
}

template <typename E>
[[noreturn]] inline void throw_with_cause(std::string_view msg, const E &cause,
                                          std::source_location loc = std::source_location::current())
{
    using namespace star::core::logging;
    const std::string full = make_message(msg, " | cause: ", cause.what());
    error("[", loc.file_name(), ":", loc.line(), " ", loc.function_name(), "] ", full);
    throw RuntimeError(full);
}

#else // C++17 fallback: use macros to capture file/line/function

[[noreturn]] inline void throw_error(const char *file, int line, const char *func, std::string_view msg)
{
    using namespace star::core::logging;
    error("[", file, ":", line, " ", func, "] ", msg);
    throw RuntimeError(std::string(msg));
}

template <typename... Args>
[[noreturn]] inline void throw_error_fmt(const char *file, int line, const char *func, std::string_view base,
                                         Args &&...args)
{
    const auto full = make_message(base, std::forward<Args>(args)...);
    throw_error(file, line, func, full);
}

template <typename E>
[[noreturn]] inline void throw_with_cause(const char *file, int line, const char *func, std::string_view msg,
                                          const E &cause)
{
    using namespace star::core::logging;
    const std::string full = make_message(msg, " | cause: ", cause.what());
    error("[", file, ":", line, " ", func, "] ", full);
    throw RuntimeError(full);
}

#endif // STAR_HAVE_SOURCE_LOCATION

// ---------------------------
// Convenience macros
// ---------------------------
#if STAR_HAVE_SOURCE_LOCATION
#define STAR_THROW(msg) ::star::core::throw_error((msg))
#define STAR_THROWF(base, ...) ::star::core::throw_error_fmt(std::source_location::current(), (base), __VA_ARGS__)
#define STAR_THROW_CAUSE(msg, cause) ::star::core::throw_with_cause((msg), (cause))
#else
#define STAR_THROW(msg) ::star::core::throw_error(__FILE__, __LINE__, __func__, (msg))
#define STAR_THROWF(base, ...) ::star::core::throw_error_fmt(__FILE__, __LINE__, __func__, (base), __VA_ARGS__)
#define STAR_THROW_CAUSE(msg, cause) ::star::core::throw_with_cause(__FILE__, __LINE__, __func__, (msg), (cause))
#endif

// ---------------------------
// Optional: install a terminate handler
// Logs any uncaught exception before aborting.
// ---------------------------
inline void install_terminate_handler()
{
    std::set_terminate([] {
        using namespace star::core::logging;
        try
        {
            if (auto ep = std::current_exception())
            {
                std::rethrow_exception(ep);
            }
            fatal("std::terminate called without active exception");
        }
        catch (const std::exception &e)
        {
            fatal("Uncaught exception: ", e.what());
        }
        catch (...)
        {
            fatal("Uncaught non-std exception");
        }
        // Ensure logs are flushed if you use async sinks
        boost::log::core::get()->flush();
        std::abort();
    });
}

} // namespace star::core