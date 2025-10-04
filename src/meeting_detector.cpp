#include "meeting_detector.h"

namespace ZoomBot {

// Static variables for time-based detection
static auto connectStartTime = std::chrono::steady_clock::now();
static bool statusDetectedSuccess = false;
static MeetingEventHandler* eventHandlerRef = nullptr;
static GMainLoop* timeoutMainLoop = nullptr;

MeetingDetector::DetectionResult MeetingDetector::checkMeetingConnection(
    ZOOM_SDK_NAMESPACE::IMeetingService* service,
    ZOOM_SDK_NAMESPACE::MeetingStatus status
) {
    DetectionResult result;
    
    std::cout << "[ACTIVE STATUS CHECK] Current meeting status: " << status;
    
    // Try to get meeting info to detect if we're actually connected
    auto meetingInfo = service->GetMeetingInfo();
    bool hasValidMeetingInfo = checkMeetingInfo(service);
    bool hasControllers = checkControllerAvailability(service);
    
    switch(status) {
        case ZOOM_SDK_NAMESPACE::MEETING_STATUS_CONNECTING:
            std::cout << " (CONNECTING)";
            // Be more conservative - only declare success if we have VERY strong indicators
            if (hasValidMeetingInfo && hasControllers && checkTimeBasedDetection()) {
                std::cout << "\n[ENHANCED DETECTION] Strong indicators suggest connection is stable!";
                result.actuallyInMeeting = true;
                result.detectionMethod = "Meeting info + controllers + time-based detection";
            } else {
                std::cout << "\n[CONNECTING] Still connecting... (info:" << (hasValidMeetingInfo ? "✓" : "✗") 
                          << " controllers:" << (hasControllers ? "✓" : "✗") << ")";
                // Continue waiting - don't declare success yet
                result.actuallyInMeeting = false;
                result.detectionMethod = "Still in CONNECTING status - waiting for proper connection";
            }
            break;
            
        case ZOOM_SDK_NAMESPACE::MEETING_STATUS_WAITINGFORHOST:
            std::cout << " (WAITING FOR HOST)";
            std::cout << "\n[ENHANCED DETECTION] Connected and waiting for host to start!";
            result.actuallyInMeeting = true;
            result.detectionMethod = "Status = WAITING_FOR_HOST (connected but waiting)";
            break;
            
        case ZOOM_SDK_NAMESPACE::MEETING_STATUS_IN_WAITING_ROOM:
            std::cout << " (IN WAITING ROOM)";
            std::cout << "\n[ENHANCED DETECTION] Bot is in waiting room, waiting for host admission!";
            result.actuallyInMeeting = true;
            result.detectionMethod = "Status = IN_WAITING_ROOM (connected but waiting for admission)";
            break;
            
        case ZOOM_SDK_NAMESPACE::MEETING_STATUS_INMEETING:
            std::cout << " (IN MEETING - SUCCESS!)";
            result.actuallyInMeeting = true;
            result.detectionMethod = "Official status = IN_MEETING";
            break;
            
        case ZOOM_SDK_NAMESPACE::MEETING_STATUS_FAILED:
            std::cout << " (FAILED - meeting join failed)";
            result.actuallyInMeeting = false;
            result.detectionMethod = "Meeting failed";
            break;
            
        default:
            std::cout << " (Other status: " << status << ")";
            result.actuallyInMeeting = false;
            result.detectionMethod = "Unknown status";
            break;
    }
    
    std::cout << std::endl;
    return result;
}

guint MeetingDetector::setupActiveDetection(
    ZOOM_SDK_NAMESPACE::IMeetingService* service,
    MeetingEventHandler* eventHandler,
    GMainLoop* mainLoop
) {
    // Reset static variables
    statusDetectedSuccess = false;
    eventHandlerRef = eventHandler;
    timeoutMainLoop = mainLoop;
    connectStartTime = std::chrono::steady_clock::now();
    
    return g_timeout_add_seconds(5, [](gpointer data) -> gboolean {
        auto* service = static_cast<ZOOM_SDK_NAMESPACE::IMeetingService*>(data);
        auto status = service->GetMeetingStatus();
        
        auto result = checkMeetingConnection(service, status);
        
        if (result.actuallyInMeeting && !statusDetectedSuccess) {
            std::cout << "\n[ENHANCED DETECTION] ✅ Bot successfully joined meeting!";
            std::cout << "\n[ENHANCED DETECTION] Detection method: " << result.detectionMethod;
            
            statusDetectedSuccess = true;
            if (eventHandlerRef) {
                eventHandlerRef->meetingJoined = true;
            }
            if (timeoutMainLoop && g_main_loop_is_running(timeoutMainLoop)) {
                std::cout << "\n[ENHANCED DETECTION] Exiting main loop due to successful detection";
                g_main_loop_quit(timeoutMainLoop);
            }
        }
        
        // Handle explicit failure
        if (status == ZOOM_SDK_NAMESPACE::MEETING_STATUS_FAILED) {
            if (eventHandlerRef) {
                eventHandlerRef->meetingFailed = true;
            }
            if (timeoutMainLoop && g_main_loop_is_running(timeoutMainLoop)) {
                g_main_loop_quit(timeoutMainLoop);
            }
        }
        
        // Continue checking until we detect success or explicit failure
        return !result.actuallyInMeeting && status != ZOOM_SDK_NAMESPACE::MEETING_STATUS_FAILED;
    }, service);
}

bool MeetingDetector::checkMeetingInfo(ZOOM_SDK_NAMESPACE::IMeetingService* service) {
    auto meetingInfo = service->GetMeetingInfo();
    std::cout << "\n[DEBUG] GetMeetingInfo() returned: " << (meetingInfo ? "Valid pointer" : "NULL");
    
    if (meetingInfo) {
        logMeetingInfo(meetingInfo);
        
        auto meetingNumber = meetingInfo->GetMeetingNumber();
        auto meetingTopic = meetingInfo->GetMeetingTopic();
        
        if (meetingNumber > 0 || (meetingTopic && strlen(meetingTopic) > 0)) {
            std::cout << "\n[ENHANCED DETECTION] ✅ Meeting info indicates successful connection!";
            return true;
        }
    }
    return false;
}

bool MeetingDetector::checkControllerAvailability(ZOOM_SDK_NAMESPACE::IMeetingService* service) {
    bool hasAudioController = false;
    bool hasVideoController = false;
    
    auto audioController = service->GetMeetingAudioController();
    if (audioController) {
        hasAudioController = true;
        std::cout << "\n[AUDIO INFO] Audio controller available";
    }
    
    auto videoController = service->GetMeetingVideoController();
    if (videoController) {
        hasVideoController = true;
        std::cout << "\n[VIDEO INFO] Video controller available";
    }
    
    return hasAudioController && hasVideoController;
}

bool MeetingDetector::checkTimeBasedDetection() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - connectStartTime).count();
    
    std::cout << "\n[TIMER] Been connecting for " << elapsed << " seconds";
    
    // If we've been "connecting" for more than 30 seconds, we're probably actually connected
    return elapsed > 30;
}

void MeetingDetector::logMeetingInfo(ZOOM_SDK_NAMESPACE::IMeetingInfo* meetingInfo) {
    auto meetingNumber = meetingInfo->GetMeetingNumber();
    auto meetingTopic = meetingInfo->GetMeetingTopic();
    auto meetingID = meetingInfo->GetMeetingID();
    auto meetingType = meetingInfo->GetMeetingType();
    
    std::cout << "\n[MEETING INFO DEBUG]:";
    std::cout << "\n  - Meeting Number: " << meetingNumber;
    std::cout << "\n  - Meeting Topic: '" << (meetingTopic ? meetingTopic : "NULL") << "'";
    std::cout << "\n  - Meeting ID: '" << (meetingID ? meetingID : "NULL") << "'";
    std::cout << "\n  - Meeting Type: " << meetingType;
}

} // namespace ZoomBot