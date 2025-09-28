#include "config.h"
#include <iostream>
#include <cstdlib>

namespace ZoomBot {

// Static member definitions
std::string Config::clientId_;
std::string Config::clientSecret_;
std::string Config::accountId_;
std::string Config::appKey_;
std::string Config::appSecret_;
uint64_t Config::meetingNumber_ = 0;
std::string Config::meetingPassword_;
std::string Config::botUsername_;
std::string Config::jwtToken_;
bool Config::loaded_ = false;

std::string Config::getEnvVar(const std::string& name, const std::string& defaultValue) {
    const char* value = std::getenv(name.c_str());
    return value ? std::string(value) : defaultValue;
}

uint64_t Config::getEnvVarUint64(const std::string& name, uint64_t defaultValue) {
    const char* value = std::getenv(name.c_str());
    if (!value) return defaultValue;
    
    try {
        return std::stoull(value);
    } catch (const std::exception& e) {
        std::cerr << "Warning: Invalid value for " << name << ": " << value << std::endl;
        return defaultValue;
    }
}

bool Config::loadFromEnvironment() {
    std::cout << "Loading configuration from environment variables..." << std::endl;

    // OAuth credentials
    clientId_ = getEnvVar("ZOOM_CLIENT_ID");
    clientSecret_ = getEnvVar("ZOOM_CLIENT_SECRET");
    accountId_ = getEnvVar("ZOOM_ACCOUNT_ID");

    // SDK credentials
    appKey_ = getEnvVar("ZOOM_APP_KEY");
    appSecret_ = getEnvVar("ZOOM_APP_SECRET");

    // Meeting configuration
    meetingNumber_ = getEnvVarUint64("ZOOM_MEETING_NUMBER");
    meetingPassword_ = getEnvVar("ZOOM_MEETING_PASSWORD");
    botUsername_ = getEnvVar("ZOOM_BOT_USERNAME", "ZoomBot");

    loaded_ = true;
    return isValid();
}

const std::string& Config::getClientId() { return clientId_; }
const std::string& Config::getClientSecret() { return clientSecret_; }
const std::string& Config::getAccountId() { return accountId_; }
const std::string& Config::getAppKey() { return appKey_; }
const std::string& Config::getAppSecret() { return appSecret_; }
uint64_t Config::getMeetingNumber() { return meetingNumber_; }
const std::string& Config::getMeetingPassword() { return meetingPassword_; }
const std::string& Config::getBotUsername() { return botUsername_; }

void Config::setMeetingNumber(uint64_t meetingNumber) {
    meetingNumber_ = meetingNumber;
}

void Config::setMeetingPassword(const std::string& password) {
    meetingPassword_ = password;
}

void Config::setJWTToken(const std::string& token) {
    jwtToken_ = token;
}

const std::string& Config::getJWTToken() {
    return jwtToken_;
}

bool Config::isValid() {
    if (!loaded_) {
        std::cerr << "Configuration not loaded. Call loadFromEnvironment() first." << std::endl;
        return false;
    }

    bool valid = true;
    
    // Check OAuth credentials
    if (clientId_.empty()) {
        std::cerr << "Missing required environment variable: ZOOM_CLIENT_ID" << std::endl;
        valid = false;
    }
    if (clientSecret_.empty()) {
        std::cerr << "Missing required environment variable: ZOOM_CLIENT_SECRET" << std::endl;
        valid = false;
    }
    if (accountId_.empty()) {
        std::cerr << "Missing required environment variable: ZOOM_ACCOUNT_ID" << std::endl;
        valid = false;
    }

    // Check SDK credentials
    if (appKey_.empty()) {
        std::cerr << "Missing required environment variable: ZOOM_APP_KEY" << std::endl;
        valid = false;
    }
    if (appSecret_.empty()) {
        std::cerr << "Missing required environment variable: ZOOM_APP_SECRET" << std::endl;
        valid = false;
    }

    // Check meeting configuration
    if (meetingNumber_ == 0) {
        std::cerr << "Missing or invalid environment variable: ZOOM_MEETING_NUMBER" << std::endl;
        valid = false;
    }
    if (meetingPassword_.empty()) {
        std::cerr << "Missing required environment variable: ZOOM_MEETING_PASSWORD" << std::endl;
        valid = false;
    }

    return valid;
}

bool Config::areCredentialsValid() {
    if (!loaded_) {
        std::cerr << "Configuration not loaded. Call loadFromEnvironment() first." << std::endl;
        return false;
    }

    bool valid = true;
    
    // Check OAuth credentials
    if (clientId_.empty()) {
        std::cerr << "Missing required environment variable: ZOOM_CLIENT_ID" << std::endl;
        valid = false;
    }
    if (clientSecret_.empty()) {
        std::cerr << "Missing required environment variable: ZOOM_CLIENT_SECRET" << std::endl;
        valid = false;
    }
    if (accountId_.empty()) {
        std::cerr << "Missing required environment variable: ZOOM_ACCOUNT_ID" << std::endl;
        valid = false;
    }

    // Check SDK credentials
    if (appKey_.empty()) {
        std::cerr << "Missing required environment variable: ZOOM_APP_KEY" << std::endl;
        valid = false;
    }
    if (appSecret_.empty()) {
        std::cerr << "Missing required environment variable: ZOOM_APP_SECRET" << std::endl;
        valid = false;
    }

    // Note: We don't check meeting configuration here - it will be provided via console

    return valid;
}

void Config::printStatus() {
    std::cout << "\n=== Configuration Status ===" << std::endl;
    std::cout << "OAuth Credentials:" << std::endl;
    std::cout << "  Client ID: " << (clientId_.empty() ? "❌ NOT SET" : "✅ SET") << std::endl;
    std::cout << "  Client Secret: " << (clientSecret_.empty() ? "❌ NOT SET" : "✅ SET") << std::endl;
    std::cout << "  Account ID: " << (accountId_.empty() ? "❌ NOT SET" : "✅ SET") << std::endl;
    
    std::cout << "SDK Credentials:" << std::endl;
    std::cout << "  App Key: " << (appKey_.empty() ? "❌ NOT SET" : "✅ SET") << std::endl;
    std::cout << "  App Secret: " << (appSecret_.empty() ? "❌ NOT SET" : "✅ SET") << std::endl;
    
    std::cout << "Meeting Configuration:" << std::endl;
    std::cout << "  Meeting Number: " << (meetingNumber_ == 0 ? "❌ NOT SET" : std::to_string(meetingNumber_)) << std::endl;
    std::cout << "  Meeting Password: " << (meetingPassword_.empty() ? "❌ NOT SET" : "✅ SET") << std::endl;
    std::cout << "  Bot Username: " << botUsername_ << std::endl;
    std::cout << "=============================" << std::endl;
}

} // namespace ZoomBot