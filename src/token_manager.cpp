#include "token_manager.h"
#include "zoom_auth.h"
#include "jwt_helper.h"
#include <iostream>
#include <chrono>
#include <sstream>

namespace ZoomBot {

TokenManager::TokenResult TokenManager::getOAuthToken(const std::string& clientId, 
                                                     const std::string& clientSecret, 
                                                     const std::string& accountId) {
    TokenResult result;
    try {
        std::cout << "[AUTH] Requesting OAuth token..." << std::endl;
        result.token = getZoomAccessToken(clientId, clientSecret, accountId);
        result.success = !result.token.empty();
        
        if (result.success) {
            std::cout << "[AUTH] ✓ OAuth token obtained" << std::endl;
        } else {
            result.errorMessage = "Empty OAuth token received";
        }
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = std::string("OAuth token request failed: ") + e.what();
        std::cerr << "[AUTH] " << result.errorMessage << std::endl;
    }
    
    return result;
}

TokenManager::TokenResult TokenManager::generateJWTToken(const std::string& appKey, 
                                                        const std::string& appSecret, 
                                                        uint64_t meetingNumber) {
    TokenResult result;
    try {
        auto header = createJWTHeader();
        auto payload = createJWTPayload(appKey, meetingNumber);
        
        result.token = ::generateJWTToken(header, payload, appSecret);
        result.success = !result.token.empty();
        
        if (result.success) {
            std::cout << "[AUTH] ✓ JWT token generated" << std::endl;
        } else {
            result.errorMessage = "Empty JWT token generated";
        }
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = std::string("JWT token generation failed: ") + e.what();
        std::cerr << "[AUTH] " << result.errorMessage << std::endl;
    }
    
    return result;
}

bool TokenManager::verifyMeetingExists(const std::string& oauthToken, uint64_t meetingNumber) {
    std::cout << "[MEETING] Verifying meeting exists..." << std::endl;
    bool exists = checkMeetingExists(oauthToken, meetingNumber);
    
    if (exists) {
        std::cout << "[MEETING] ✓ Meeting verified" << std::endl;
    } else {
        std::cerr << "[MEETING] ✗ Meeting not found or not accessible" << std::endl;
    }
    
    return exists;
}

nlohmann::json TokenManager::createJWTHeader() {
    return nlohmann::json{
        {"alg", "HS256"},
        {"typ", "JWT"}
    };
}

nlohmann::json TokenManager::createJWTPayload(const std::string& appKey, uint64_t meetingNumber) {
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    std::ostringstream oss;
    oss << meetingNumber;
    std::string meetingNumberStr = oss.str();
    
    return nlohmann::json{
        {"appKey", appKey},
        {"exp", now + 3600},
        {"iat", now},
        {"mn", meetingNumberStr},
        {"role", 0},
        {"sdkKey", appKey},
        {"tokenExp", now + 3600}
    };
}

}