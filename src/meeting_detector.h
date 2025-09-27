#pragma once

#include <iostream>
#include <chrono>
#include <glib.h>
#include "zoom_sdk.h"
#include "zoom_sdk_def.h"
#include "meeting_service_interface.h"
#include "meeting_event_handler.h"

namespace ZoomBot {

class MeetingDetector {
public:
    struct DetectionResult {
        bool actuallyInMeeting = false;
        std::string detectionMethod;
    };

    static DetectionResult checkMeetingConnection(
        ZOOM_SDK_NAMESPACE::IMeetingService* service,
        ZOOM_SDK_NAMESPACE::MeetingStatus status
    );

    static guint setupActiveDetection(
        ZOOM_SDK_NAMESPACE::IMeetingService* service,
        MeetingEventHandler* eventHandler,
        GMainLoop* mainLoop
    );

private:
    static bool checkMeetingInfo(ZOOM_SDK_NAMESPACE::IMeetingService* service);
    static bool checkControllerAvailability(ZOOM_SDK_NAMESPACE::IMeetingService* service);
    static bool checkTimeBasedDetection();
    static void logMeetingInfo(ZOOM_SDK_NAMESPACE::IMeetingInfo* meetingInfo);
};

} // namespace ZoomBot