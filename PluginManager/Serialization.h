#pragma once

#include "libs/includes/nlohmann/json.hpp"

#define NLOHMANN_JSON_FROM_OPTIONAL(v1) if (const auto it = nlohmann_json_j.find(#v1); it != nlohmann_json_j.end()) {it->get_to(nlohmann_json_t.v1);}

#define NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_OPTIONAL(Type, ...)  \
    inline void to_json(nlohmann::json& nlohmann_json_j, const Type& nlohmann_json_t) { NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_TO, __VA_ARGS__)) } \
    inline void from_json(const nlohmann::json& nlohmann_json_j, Type& nlohmann_json_t) { NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_FROM_OPTIONAL, __VA_ARGS__)) }


struct PluginInstallResult
{
		std::string dll;
		bool already_exists = false;
		bool was_loaded = true;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_OPTIONAL(PluginInstallResult, dll, already_exists, was_loaded)