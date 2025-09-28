#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace ZoomBot {
    /**
     * Handles OAuth and JWT token generation with minimal verbose output
     */
    class TokenManager {
    public:
        struct TokenResult {
            bool success;
            std::string token;
            std::string errorMessage;
        };

        /**
         * Get OAuth access token
         */
        static TokenResult getOAuthToken(const std::string& clientId, 
                                       const std::string& clientSecret, 
                                       const std::string& accountId);

        /**
         * Generate JWT token for SDK authentication
         */
        static TokenResult generateJWTToken(const std::string& appKey, 
                                          const std::string& appSecret, 
                                          uint64_t meetingNumber);

        /**
         * Verify meeting exists (with minimal output)
         */
        static bool verifyMeetingExists(const std::string& oauthToken, uint64_t meetingNumber);

    private:
        static nlohmann::json createJWTHeader();
        static nlohmann::json createJWTPayload(const std::string& appKey, uint64_t meetingNumber);
    };
}