#include "config.h"

#include <rapidjson/error/en.h>

#include <fstream>
#include <sstream>

Result<Config::Values> Config::load() {
    std::ifstream file(m_path);
    if (!file.good()) {
        return Result<Values>::Err(
            Error(__func__, "failed to open file \"" + m_path + "\""));
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    rapidjson::Document doc;
    rapidjson::ParseResult res = doc.Parse(ss.str().c_str());

    if (!res) {
        std::string err_str =
            std::string(rapidjson::GetParseError_En(res.Code()));
        return Result<Values>::Err(Error(__func__, err_str));
    }

    Result<std::string> log_level_str_res = read_str(doc, "log_level");
    if (!log_level_str_res.is_ok()) {
        return Result<Values>::Err(Error(__func__, log_level_str_res));
    }

    Result<std::string> driver_str_res = read_str(doc, "driver");
    if (!driver_str_res.is_ok()) {
        return Result<Values>::Err(Error(__func__, driver_str_res));
    }

    Result<std::string> gateway_url_res = read_str(doc, "gateway_url");
    if (!gateway_url_res.is_ok()) {
        return Result<Values>::Err(Error(__func__, gateway_url_res));
    }

    Result<std::string> gateway_namespace_res =
        read_str(doc, "gateway_namespace");
    if (!gateway_namespace_res.is_ok()) {
        return Result<Values>::Err(Error(__func__, gateway_namespace_res));
    }

    Result<int> reconn_delay_res = read_int(doc, "reconn_delay");
    if (!reconn_delay_res.is_ok()) {
        return Result<Values>::Err(Error(__func__, reconn_delay_res));
    }

    Result<int> reconn_attempts_res = read_int(doc, "reconn_attempts");
    if (!reconn_attempts_res.is_ok()) {
        return Result<Values>::Err(Error(__func__, reconn_attempts_res));
    }

    Values values;
    values.m_log_level_str = log_level_str_res.unwrap();
    values.m_driver_str = driver_str_res.unwrap();

    std::string gateway_url = gateway_url_res.unwrap();
    std::string gateway_namespace = gateway_namespace_res.unwrap();
    int reconn_delay = reconn_delay_res.unwrap();
    int reconn_attempts = reconn_attempts_res.unwrap();

    Result<> fe_res = for_each(
        doc, "plugs", [&](const rapidjson::Document& plug_doc) -> Result<> {
            Result<std::string> model_str_res = read_str(plug_doc, "model");
            if (!model_str_res.is_ok()) {
                return Result<>::Err(Error(__func__, model_str_res));
            }

            Result<int> gpio_pin_res = read_int(plug_doc, "gpio_pin");
            if (!gpio_pin_res.is_ok()) {
                return Result<>::Err(Error(__func__, gpio_pin_res));
            }

            Result<int> lock_duration_res = read_int(plug_doc, "lock_duration");
            if (!lock_duration_res.is_ok()) {
                return Result<>::Err(Error(__func__, lock_duration_res));
            }

            Result<std::string> device_id_res = read_str(plug_doc, "device_id");
            if (!device_id_res.is_ok()) {
                return Result<>::Err(Error(__func__, device_id_res));
            }

            Result<std::string> secret_res = read_str(plug_doc, "secret");
            if (!secret_res.is_ok()) {
                return Result<>::Err(Error(__func__, secret_res));
            }

            Plug::Config plug_config;
            plug_config.m_model_str = model_str_res.unwrap();
            plug_config.m_gpio_pin = gpio_pin_res.unwrap();
            plug_config.m_lock_duration = lock_duration_res.unwrap();

            plug_config.m_device_id = device_id_res.unwrap();
            plug_config.m_secret = secret_res.unwrap();

            plug_config.m_gateway_url = gateway_url;
            plug_config.m_gateway_namespace = gateway_namespace;
            plug_config.m_reconn_delay = reconn_delay;
            plug_config.m_reconn_attempts = reconn_attempts;

            values.m_plugs.push_back(plug_config);

            return Result<>::Ok(None{});
        });
    if (!fe_res.is_ok()) {
        return Result<Values>::Err(Error(__func__, fe_res));
    }

    return Result<Values>::Ok(values);
}

std::string Config::to_str() { return ""; }

Result<std::string> Config::read_str(const rapidjson::Document& doc,
                                     const std::string& key) {
    if (!doc.HasMember(key.c_str()) || !doc[key.c_str()].IsString()) {
        return Result<std::string>::Err(
            Error(__func__, "failed to read required string \"" + key + "\""));
    }

    return Result<std::string>::Ok(std::string(doc[key.c_str()].GetString()));
}

Result<int> Config::read_int(const rapidjson::Document& doc,
                             const std::string& key) {
    if (!doc.HasMember(key.c_str()) || !doc[key.c_str()].IsInt()) {
        return Result<int>::Err(
            Error(__func__, "failed to read required integer \"" + key + "\""));
    }

    return Result<int>::Ok(doc[key.c_str()].GetInt());
}

Result<bool> Config::read_bool(const rapidjson::Document& doc,
                               const std::string& key) {
    if (!doc.HasMember(key.c_str()) || !doc[key.c_str()].IsBool()) {
        return Result<bool>::Err(
            Error(__func__, "failed to read required boolean \"" + key + "\""));
    }

    return Result<bool>::Ok(doc[key.c_str()].GetBool());
}

Result<> Config::read_object(const rapidjson::Document& doc,
                             const std::string& key,
                             rapidjson::Document& res_doc_ref) {
    if (!doc.HasMember(key.c_str()) || !doc[key.c_str()].IsObject()) {
        return Result<>::Err(
            Error(__func__, "failed to read required object \"" + key + "\""));
    }

    res_doc_ref.CopyFrom(doc[key.c_str()], res_doc_ref.GetAllocator());
    return Result<>::Ok(None{});
}

Result<>
Config::for_each(const rapidjson::Document& doc, const std::string& key,
                 std::function<Result<>(const rapidjson::Document&)> callback) {
    if (!doc.HasMember(key.c_str()) || !doc[key.c_str()].IsArray()) {
        return Result<>::Err(
            Error(__func__, "failed to read required array \"" + key + "\""));
    }

    for (const auto& member : doc[key.c_str()].GetArray()) {
        if (!member.IsObject()) {
            return Result<>::Err(Error(
                __func__, "array \"" + key + "\" contains invalid member"));
        }

        rapidjson::Document res_doc;
        res_doc.CopyFrom(member, res_doc.GetAllocator());

        Result<> cb_res = callback(res_doc);
        if (!cb_res.is_ok()) {
            return Result<>::Err(Error(__func__, cb_res));
        }
    }

    return Result<>::Ok(None{});
}