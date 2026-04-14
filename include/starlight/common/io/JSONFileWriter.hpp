#pragma once


#include <nlohmann/json.hpp>

namespace star::common::io
{
class JSONFileWriter
{
  public:
    explicit JSONFileWriter(nlohmann::json jData) : m_jData(std::move(jData))
    {
    }

    int operator()(const std::string &path);

    private:
    nlohmann::json m_jData; 
};
}