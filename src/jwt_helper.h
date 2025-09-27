#pragma once
#include <string>
#include <nlohmann/json.hpp>

std::string base64url_encode(const std::string& input);
std::string hmacSha256(const std::string& data, const std::string& secret);
std::string generateJWTToken(const nlohmann::json& header, 
                           const nlohmann::json& payload,
                           const std::string& secret);