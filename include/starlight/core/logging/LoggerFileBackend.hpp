#pragma once

#include <boost/log/expressions.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

#include <filesystem>

namespace star::core::logging
{
namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

struct LoggerFileBackend : public sinks::basic_sink_backend<sinks::concurrent_feeding>
{
    explicit LoggerFileBackend(std::filesystem::path baseDir) : m_baseDir(std::move(baseDir)) {};

    void consume(logging::record_view const& rec); 
  private:
    std::filesystem::path m_baseDir;

    std::ofstream &getStream(); 
    std::ofstream openFile();
};
} // namespace star::core::logging