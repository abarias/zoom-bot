#pragma once

#include <string>
#include <cstdint>

namespace ZoomBot {

/**
 * @brief Configuration manager for environment variables and secrets
 */
class Config {
public:
    /**
     * @brief Load configuration from environment variables
     * @return true if all required variables are present, false otherwise
     */
    static bool loadFromEnvironment();

    /**
     * @brief Get OAuth credentials
     */
    static const std::string& getClientId();
    static const std::string& getClientSecret();
    static const std::string& getAccountId();

    /**
     * @brief Get SDK credentials  
     */
    static const std::string& getAppKey();
    static const std::string& getAppSecret();

    /**
     * @brief Get meeting configuration
     */
    static uint64_t getMeetingNumber();
    static const std::string& getMeetingPassword();
    static const std::string& getBotUsername();

    /**
     * @brief Override meeting configuration (for console input)
     */
    static void setMeetingNumber(uint64_t meetingNumber);
    static void setMeetingPassword(const std::string& password);

    /**
     * @brief Validate that all required configuration is loaded
     * @return true if configuration is valid, false otherwise
     */
    static bool isValid();

    /**
     * @brief Validate only credentials (OAuth and SDK), not meeting details
     * @return true if credentials are valid, false otherwise
     */
    static bool areCredentialsValid();

    /**
     * @brief Print configuration status (without sensitive values)
     */
    static void printStatus();

private:
    static std::string getEnvVar(const std::string& name, const std::string& defaultValue = "");
    static uint64_t getEnvVarUint64(const std::string& name, uint64_t defaultValue = 0);

    // OAuth credentials
    static std::string clientId_;
    static std::string clientSecret_;
    static std::string accountId_;

    // SDK credentials
    static std::string appKey_;
    static std::string appSecret_;

    // Meeting configuration
    static uint64_t meetingNumber_;
    static std::string meetingPassword_;
    static std::string botUsername_;

    static bool loaded_;
};

} // namespace ZoomBot