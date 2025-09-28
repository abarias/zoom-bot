#include "audio_manager.h"
#include "meeting_service_components/meeting_audio_interface.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace ZoomBot {

AudioManager::AudioSetupResult AudioManager::setupAudioCapture(ZOOM_SDK_NAMESPACE::IMeetingService* meetingService,
                                                              AudioRawHandler& audioHandler) {
    AudioSetupResult result;
    result.success = false;
    result.recordingEnabled = false;
    result.streamingEnabled = false;

    audioHandler.setMeetingService(meetingService);

    // Join VoIP first
    std::cout << "[AUDIO] Joining VoIP..." << std::endl;
    if (!joinVoIP(meetingService)) {
        result.statusMessage = "VoIP join failed";
        return result;
    }

    // Request recording permission (simplified output)
    std::cout << "[AUDIO] Requesting recording permission..." << std::endl;
    bool permissionRequested = audioHandler.requestRecordingPermission();
    
    if (!permissionRequested) {
        std::cout << "[AUDIO] Recording permission not available - attempting direct subscription" << std::endl;
    }

    // Attempt audio subscription
    std::cout << "[AUDIO] Subscribing to audio data..." << std::endl;
    bool subscribed = audioHandler.subscribe(false);
    
    if (subscribed) {
        result.success = true;
        result.recordingEnabled = true;
        result.statusMessage = "Audio capture enabled";
        std::cout << "[AUDIO] ✓ Audio recording enabled" << std::endl;

        // Enable streaming
        std::cout << "[AUDIO] Enabling streaming..." << std::endl;
        result.streamingEnabled = audioHandler.enableStreaming("tcp", "localhost:8888");
        if (result.streamingEnabled) {
            std::cout << "[AUDIO] ✓ Streaming enabled" << std::endl;
        } else {
            std::cout << "[AUDIO] ⚠ Streaming failed - file recording only" << std::endl;
        }
    } else {
        result.statusMessage = "Audio subscription failed - no recording permission";
        std::cout << "[AUDIO] ✗ Audio subscription failed" << std::endl;
    }

    return result;
}

bool AudioManager::joinVoIP(ZOOM_SDK_NAMESPACE::IMeetingService* meetingService, int timeoutSeconds) {
    if (!meetingService) return false;

    auto* audioCtrl = meetingService->GetMeetingAudioController();
    if (!audioCtrl) return false;

    // Configure audio settings
    audioCtrl->EnablePlayMeetingAudio(false); // Disable local audio playback
    auto joinResult = audioCtrl->JoinVoip();
    
    if (joinResult != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS) {
        std::cerr << "[AUDIO] VoIP join failed: " << joinResult << std::endl;
        return false;
    }

    // Wait for VoIP connection
    return waitForVoipJoin(meetingService, timeoutSeconds);
}

bool AudioManager::waitForVoipJoin(ZOOM_SDK_NAMESPACE::IMeetingService* meetingService, int timeoutSeconds) {
    const int intervalMs = 500;
    int waitedMs = 0;
    
    while (waitedMs < timeoutSeconds * 1000) {
        if (isVoipJoined(meetingService)) {
            std::cout << "[AUDIO] ✓ VoIP joined" << std::endl;
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
        waitedMs += intervalMs;
    }
    
    std::cerr << "[AUDIO] VoIP join timeout" << std::endl;
    return false;
}

bool AudioManager::isVoipJoined(ZOOM_SDK_NAMESPACE::IMeetingService* meetingService) {
    if (!meetingService) return false;
    auto* pc = meetingService->GetMeetingParticipantsController();
    if (!pc) return false;
    auto* self = pc->GetMySelfUser();
    if (!self) return false;
    auto at = self->GetAudioJoinType();
    return at == ZOOM_SDK_NAMESPACE::AUDIOTYPE_VOIP || at == ZOOM_SDK_NAMESPACE::AUDIOTYPE_PHONE;
}

}