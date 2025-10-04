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

    // Check current meeting status first
    auto currentStatus = meetingService->GetMeetingStatus();
    std::cout << "[AUDIO] Current meeting status before VoIP join: " << currentStatus << std::endl;
    
    // If still CONNECTING, wait for stability
    if (currentStatus == ZOOM_SDK_NAMESPACE::MEETING_STATUS_CONNECTING) {
        std::cout << "[AUDIO] Meeting still CONNECTING - waiting for stable status..." << std::endl;
        if (!waitForMeetingStable(meetingService, 15)) {
            std::cout << "[AUDIO] ⚠ Meeting did not stabilize - VoIP join will likely fail" << std::endl;
            std::cout << "[AUDIO] Current status: " << meetingService->GetMeetingStatus() << std::endl;
            return false;  // Don't attempt VoIP join if meeting isn't stable
        }
    } else {
        std::cout << "[AUDIO] Meeting status is stable, proceeding with VoIP join..." << std::endl;
    }

    auto* audioCtrl = meetingService->GetMeetingAudioController();
    if (!audioCtrl) {
        std::cerr << "[AUDIO] Audio controller not available" << std::endl;
        return false;
    }

    std::cout << "[AUDIO] Configuring audio settings..." << std::endl;
    // Configure audio settings for container environment
    audioCtrl->EnablePlayMeetingAudio(false); // Disable local audio playback to avoid feedback
    
    // Try to set audio device if possible (may help with virtual audio)
    // Note: In container environment, this may not be necessary but doesn't hurt
    
    std::cout << "[AUDIO] Attempting to join VoIP..." << std::endl;
    auto joinResult = audioCtrl->JoinVoip();
    
    if (joinResult != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS) {
        std::cerr << "[AUDIO] VoIP join failed with error code: " << joinResult;
        switch(joinResult) {
            case 2: // SDKERR_WRONG_USAGE
                std::cerr << " (WRONG_USAGE - May need to wait longer or meeting not ready)";
                break;
            case 17: // SDKERR_NO_AUDIODEVICE_ISFOUND  
                std::cerr << " (NO_AUDIODEVICE_FOUND - Virtual audio devices may not be detected)";
                break;
            default:
                std::cerr << " (Unknown error)";
                break;
        }
        std::cerr << std::endl;
        return false;
    }

    std::cout << "[AUDIO] VoIP join request sent, waiting for connection..." << std::endl;
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

bool AudioManager::waitForMeetingStable(ZOOM_SDK_NAMESPACE::IMeetingService* meetingService, int timeoutSeconds) {
    const int intervalMs = 500;
    int waitedMs = 0;
    
    std::cout << "[AUDIO] Waiting for meeting to reach stable status..." << std::endl;
    
    while (waitedMs < timeoutSeconds * 1000) {
        auto status = meetingService->GetMeetingStatus();
        std::cout << "[AUDIO] Current status: " << status << std::endl;
        
        // Check if meeting is in a stable state (not connecting)
        if (status == ZOOM_SDK_NAMESPACE::MEETING_STATUS_INMEETING) {
            std::cout << "[AUDIO] ✓ Meeting is stable (INMEETING)" << std::endl;
            return true;
        }
        
        // Also accept these statuses as potentially stable
        if (status == ZOOM_SDK_NAMESPACE::MEETING_STATUS_WAITINGFORHOST) {
            std::cout << "[AUDIO] ✓ Meeting is stable (WAITING_FOR_HOST)" << std::endl;
            return true;
        }
        
        if (status == ZOOM_SDK_NAMESPACE::MEETING_STATUS_IN_WAITING_ROOM) {
            std::cout << "[AUDIO] ✓ Meeting is stable (IN_WAITING_ROOM)" << std::endl;
            return true;
        }
        
        // For CONNECTING status, wait longer but eventually timeout
        if (status == ZOOM_SDK_NAMESPACE::MEETING_STATUS_CONNECTING) {
            if (waitedMs > 15000) { // After 15 seconds of CONNECTING, give up
                std::cout << "[AUDIO] ⚠ Still CONNECTING after 15s - meeting may not stabilize" << std::endl;
                return false;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
        waitedMs += intervalMs;
    }
    
    std::cout << "[AUDIO] ⚠ Meeting stability timeout after " << timeoutSeconds << "s" << std::endl;
    return false;
}

}