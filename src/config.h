#pragma once

#include "plug.h"

#include <homecontroller/util/result.h>

#include <rapidjson/document.h>
#include <vector>

class Config {
  public:
    struct Values {
        std::string m_log_level_str;
        std::string m_driver_str;
        std::vector<Plug::Config> m_plugs;
    };

    Config(const std::string& path) : m_path(path) {}
    ~Config() {}

    Result<Values> load();
    std::string to_str();

  private:
    Result<std::string> read_str(const rapidjson::Document& doc,
                                 const std::string& key);
    Result<int> read_int(const rapidjson::Document& doc,
                         const std::string& key);
    Result<bool> read_bool(const rapidjson::Document& doc,
                           const std::string& key);
    Result<> read_object(const rapidjson::Document& doc, const std::string& key,
                         rapidjson::Document& res_doc_ref);

    Result<>
    for_each(const rapidjson::Document& doc, const std::string& key,
             std::function<Result<>(const rapidjson::Document&)> callback);

    std::string m_path;
};