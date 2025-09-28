#pragma once

#include <string>

namespace ZoomBot {
    /**
     * Handles meeting setup and user input with clean interface
     */
    class MeetingSetup {
    public:
        struct MeetingDetails {
            bool success;
            std::string meetingNumber;
            std::string password;
            std::string errorMessage;
        };

        /**
         * Get meeting details from console input with validation
         */
        static MeetingDetails getMeetingDetailsFromConsole();

    private:
        static std::string trim(const std::string& str);
        static std::string parseMeetingNumber(const std::string& input);
        static bool confirmDetails(const std::string& meetingNumber, const std::string& password);
    };
}