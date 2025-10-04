#pragma once

#include "audio_raw_handler.h"
#include "meeting_service_interface.h"

namespace ZoomBot {
    /**
     * Manages audio subscription and recording with simplified interface
     */
    class AudioManager {
    public:
        struct AudioSetupResult {
            bool success;
            bool recordingEnabled;
            bool streamingEnabled;
            std::string statusMessage;
        };

        /**
         * Initialize audio handler and attempt subscription
         */
        static AudioSetupResult setupAudioCapture(ZOOM_SDK_NAMESPACE::IMeetingService* meetingService,
                                                 AudioRawHandler& audioHandler);

        /**
         * Join VoIP with timeout
         */
        static bool joinVoIP(ZOOM_SDK_NAMESPACE::IMeetingService* meetingService, int timeoutSeconds = 20);

    private:
        static bool waitForVoipJoin(ZOOM_SDK_NAMESPACE::IMeetingService* meetingService, int timeoutSeconds);
        static bool isVoipJoined(ZOOM_SDK_NAMESPACE::IMeetingService* meetingService);
        static bool waitForMeetingStable(ZOOM_SDK_NAMESPACE::IMeetingService* meetingService, int timeoutSeconds);
    };
}