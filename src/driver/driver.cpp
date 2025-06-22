#include "driver.h"

#include <map>

Result<Driver::Model> Driver::str_to_model(const std::string& str) {
    static std::map<std::string, Model> str_to_model_map = {
        {"PLUG_V1", Model::PLUG_V1}};

    auto mit = str_to_model_map.find(str);
    if (mit == str_to_model_map.end()) {
        return Result<Model>::Err(Error(__func__, "invalid model name"));
    }

    return Result<Model>::Ok(mit->second);
}