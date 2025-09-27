#pragma once
#include <string>
#include <vector>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

// Base64 encoding function
std::string base64_encode(const std::string& input);

// Helper class for HTTP requests
class HttpClient {
public:
    static std::string request(const std::string& url, 
                             const std::vector<std::string>& headers, 
                             bool isPost = false, 
                             const std::string& postFields = "");
};

// Main API functions
std::string getZoomAccessToken(const std::string& clientId,
                               const std::string& clientSecret,
                               const std::string& accountId);

std::string getZoomZAK(const std::string& accessToken);

bool checkMeetingExists(const std::string& accessToken, uint64_t meetingNumber);

// Get meeting details including numeric ID if available
uint64_t getMeetingNumericId(const std::string& accessToken, uint64_t meetingNumber);

// Generate JWT token
std::string generateJWTToken(const nlohmann::json& header, 
                           const nlohmann::json& payload,
                           const std::string& secret);
