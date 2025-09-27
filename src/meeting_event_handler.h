#pragma once

#include <iostream>
#include <glib.h>
#include "zoom_sdk.h"
#include "zoom_sdk_def.h"
#include "meeting_service_interface.h"
#include "meeting_service_components/meeting_recording_interface.h"

namespace ZoomBot {

class MeetingEventHandler : public ZOOM_SDK_NAMESPACE::IMeetingServiceEvent,
                          public ZOOM_SDK_NAMESPACE::IMeetingRecordingCtrlEvent {
public:
    GMainLoop* mainLoop = nullptr;
    bool meetingJoined = false;
    bool meetingFailed = false;
    bool recordingPermissionGranted = false;
    bool recordingPermissionDenied = false;
    
    explicit MeetingEventHandler(GMainLoop* loop);
    
    // IMeetingServiceEvent overrides
    void onMeetingStatusChanged(ZOOM_SDK_NAMESPACE::MeetingStatus status, int result) override;
    void onMeetingStatisticsWarningNotification(ZOOM_SDK_NAMESPACE::StatisticsWarningType type) override;
    void onMeetingParameterNotification(const ZOOM_SDK_NAMESPACE::MeetingParameter* meeting_param) override;
    void onSuspendParticipantsActivities() override;
    void onAICompanionActiveChangeNotice(bool bActive) override;
    void onMeetingTopicChanged(const zchar_t* sTopic) override;
    void onMeetingFullToWatchLiveStream(const zchar_t* sLiveStreamUrl) override;

    // IMeetingRecordingCtrlEvent overrides
    void onRecordingStatus(ZOOM_SDK_NAMESPACE::RecordingStatus status) override;
    void onCloudRecordingStatus(ZOOM_SDK_NAMESPACE::RecordingStatus status) override;
    void onRecordPrivilegeChanged(bool bCanRec) override;
    void onLocalRecordingPrivilegeRequestStatus(ZOOM_SDK_NAMESPACE::RequestLocalRecordingStatus status) override;
    void onRequestCloudRecordingResponse(ZOOM_SDK_NAMESPACE::RequestStartCloudRecordingStatus status) override;
    void onLocalRecordingPrivilegeRequested(ZOOM_SDK_NAMESPACE::IRequestLocalRecordingPrivilegeHandler* handler) override;
    void onStartCloudRecordingRequested(ZOOM_SDK_NAMESPACE::IRequestStartCloudRecordingHandler* handler) override;
    void onCloudRecordingStorageFull(time_t gracePeriodDate) override;
    void onEnableAndStartSmartRecordingRequested(ZOOM_SDK_NAMESPACE::IRequestEnableAndStartSmartRecordingHandler* handler) override;
    void onSmartRecordingEnableActionCallback(ZOOM_SDK_NAMESPACE::ISmartRecordingEnableActionHandler* handler) override;
    void onTranscodingStatusChanged(ZOOM_SDK_NAMESPACE::TranscodingStatus status, const zchar_t* path) override;

private:
    void logMeetingStatus(ZOOM_SDK_NAMESPACE::MeetingStatus status, int result);
    void logMeetingFailureReason(int result);
};

} // namespace ZoomBot