#include "meeting_event_handler.h"

namespace ZoomBot {

MeetingEventHandler::MeetingEventHandler(GMainLoop* loop) : mainLoop(loop) {}

void MeetingEventHandler::onMeetingStatusChanged(ZOOM_SDK_NAMESPACE::MeetingStatus status, int result) {
    std::cout << "\n[CALLBACK] onMeetingStatusChanged called! Status: " << status << ", Result: " << result << std::endl;
    std::cout << "[CALLBACK] Meeting status changed to: ";
    
    logMeetingStatus(status, result);
    
    switch (status) {
        case ZOOM_SDK_NAMESPACE::MEETING_STATUS_INMEETING:
            meetingJoined = true;
            if (mainLoop && g_main_loop_is_running(mainLoop)) {
                std::cout << " [Exiting main loop]";
                g_main_loop_quit(mainLoop);
            }
            break;
        case ZOOM_SDK_NAMESPACE::MEETING_STATUS_FAILED:
            meetingFailed = true;
            if (result != 0) {
                logMeetingFailureReason(result);
            }
            if (mainLoop && g_main_loop_is_running(mainLoop)) {
                g_main_loop_quit(mainLoop);
            }
            break;
        default:
            break;
    }
    std::cout << std::endl;
}

void MeetingEventHandler::logMeetingStatus(ZOOM_SDK_NAMESPACE::MeetingStatus status, int result) {
    switch (status) {
        case ZOOM_SDK_NAMESPACE::MEETING_STATUS_IDLE:
            std::cout << "IDLE"; 
            break;
        case ZOOM_SDK_NAMESPACE::MEETING_STATUS_CONNECTING:
            std::cout << "CONNECTING (Still connecting, please wait...)";
            if (result != 0) {
                std::cout << " [Result code: " << result << "]";
            }
            break;
        case ZOOM_SDK_NAMESPACE::MEETING_STATUS_WAITINGFORHOST:
            std::cout << "WAITING FOR HOST (Host hasn't started the meeting yet, continuing to wait...)";
            break;
        case ZOOM_SDK_NAMESPACE::MEETING_STATUS_INMEETING:
            std::cout << "IN MEETING - SUCCESS!";
            break;
        case ZOOM_SDK_NAMESPACE::MEETING_STATUS_FAILED:
            std::cout << "FAILED";
            break;
        default:
            std::cout << "UNKNOWN STATUS (" << status << ")";
            break;
    }
}

void MeetingEventHandler::logMeetingFailureReason(int result) {
    std::cout << " - Failure reason: ";
    switch (result) {
        case ZOOM_SDK_NAMESPACE::MEETING_FAIL_PASSWORD_ERR:
            std::cout << " (Password error)"; 
            break;
        case ZOOM_SDK_NAMESPACE::MEETING_FAIL_MEETING_NOT_EXIST:
            std::cout << " (Meeting does not exist)"; 
            break;
        case ZOOM_SDK_NAMESPACE::MEETING_FAIL_MEETING_NOT_START:
            std::cout << " (Meeting has not started)"; 
            break;
        case ZOOM_SDK_NAMESPACE::MEETING_FAIL_MEETING_OVER:
            std::cout << " (Meeting is over)"; 
            break;
        default:
            std::cout << " (Code: " << result << ")";
            break;
    }
}

// Empty implementations for required virtual functions
void MeetingEventHandler::onMeetingStatisticsWarningNotification(ZOOM_SDK_NAMESPACE::StatisticsWarningType type) {}
void MeetingEventHandler::onMeetingParameterNotification(const ZOOM_SDK_NAMESPACE::MeetingParameter* meeting_param) {}
void MeetingEventHandler::onSuspendParticipantsActivities() {}
void MeetingEventHandler::onAICompanionActiveChangeNotice(bool bActive) {}
void MeetingEventHandler::onMeetingTopicChanged(const zchar_t* sTopic) {}
void MeetingEventHandler::onMeetingFullToWatchLiveStream(const zchar_t* sLiveStreamUrl) {}

// IMeetingRecordingCtrlEvent implementations
void MeetingEventHandler::onRecordingStatus(ZOOM_SDK_NAMESPACE::RecordingStatus status) {
    std::cout << "\n[CALLBACK] Recording status changed: ";
    switch(status) {
        case ZOOM_SDK_NAMESPACE::Recording_Start:
            std::cout << "STARTED - Local recording is now active!";
            break;
        case ZOOM_SDK_NAMESPACE::Recording_Stop:
            std::cout << "STOPPED";
            break;
        case ZOOM_SDK_NAMESPACE::Recording_DiskFull:
            std::cout << "DISK_FULL";
            break;
        case ZOOM_SDK_NAMESPACE::Recording_Pause:
            std::cout << "PAUSED";
            break;
        case ZOOM_SDK_NAMESPACE::Recording_Connecting:
            std::cout << "CONNECTING";
            break;
        case ZOOM_SDK_NAMESPACE::Recording_Fail:
            std::cout << "FAILED";
            break;
        default:
            std::cout << "UNKNOWN (" << status << ")";
            break;
    }
    std::cout << std::endl;
}

void MeetingEventHandler::onCloudRecordingStatus(ZOOM_SDK_NAMESPACE::RecordingStatus status) {
    std::cout << "\n[CALLBACK] Cloud recording status: " << status << std::endl;
}

void MeetingEventHandler::onRecordPrivilegeChanged(bool bCanRec) {
    std::cout << "\n[CALLBACK] Record privilege changed: " << (bCanRec ? "CAN_RECORD" : "CANNOT_RECORD") << std::endl;
}

void MeetingEventHandler::onLocalRecordingPrivilegeRequestStatus(ZOOM_SDK_NAMESPACE::RequestLocalRecordingStatus status) {
    std::cout << "\n[CALLBACK] Recording permission status: ";
    switch(status) {
        case ZOOM_SDK_NAMESPACE::RequestLocalRecording_Granted:
            std::cout << "GRANTED - Recording permission approved by host!";
            recordingPermissionGranted = true;
            recordingPermissionDenied = false;
            break;
        case ZOOM_SDK_NAMESPACE::RequestLocalRecording_Denied:
            std::cout << "DENIED - Recording permission denied by host";
            recordingPermissionGranted = false;
            recordingPermissionDenied = true;
            break;
        case ZOOM_SDK_NAMESPACE::RequestLocalRecording_Timeout:
            std::cout << "TIMEOUT - Host did not respond to recording permission request";
            recordingPermissionGranted = false;
            recordingPermissionDenied = false; // Timeout is not explicit denial
            break;
        default:
            std::cout << "UNKNOWN STATUS (" << status << ")";
            break;
    }
    std::cout << std::endl;
}

void MeetingEventHandler::onRequestCloudRecordingResponse(ZOOM_SDK_NAMESPACE::RequestStartCloudRecordingStatus status) {}
void MeetingEventHandler::onLocalRecordingPrivilegeRequested(ZOOM_SDK_NAMESPACE::IRequestLocalRecordingPrivilegeHandler* handler) {}

// Additional required methods for IMeetingRecordingCtrlEvent
void MeetingEventHandler::onStartCloudRecordingRequested(ZOOM_SDK_NAMESPACE::IRequestStartCloudRecordingHandler* handler) {}
void MeetingEventHandler::onCloudRecordingStorageFull(time_t gracePeriodDate) {}
void MeetingEventHandler::onEnableAndStartSmartRecordingRequested(ZOOM_SDK_NAMESPACE::IRequestEnableAndStartSmartRecordingHandler* handler) {}
void MeetingEventHandler::onSmartRecordingEnableActionCallback(ZOOM_SDK_NAMESPACE::ISmartRecordingEnableActionHandler* handler) {}
void MeetingEventHandler::onTranscodingStatusChanged(ZOOM_SDK_NAMESPACE::TranscodingStatus status, const zchar_t* path) {}

} // namespace ZoomBot