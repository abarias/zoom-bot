#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <sstream>
#include <csignal>
#include <atomic>
#include <nlohmann/json.hpp>
#include <glib.h>
#include <sys/signalfd.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>

// Zoom SDK includes
#include "zoom_sdk.h"
#include "zoom_sdk_def.h"
#include "meeting_service_interface.h"
#include "meeting_service_components/meeting_audio_interface.h"

// Our refactored components
#include "auth_event_handler.h"
#include "meeting_event_handler.h"
#include "meeting_detector.h"
#include "sdk_initializer.h"
#include "zoom_auth.h"
#include "jwt_helper.h"
#include "audio_raw_handler.h"
#include "config.h"

using namespace ZoomBot;

// Global variables for clean shutdown
std::atomic<bool> shouldExit{false};
ZoomBot::AudioRawHandler* globalAudioHandler = nullptr;
ZOOM_SDK_NAMESPACE::IMeetingService* globalMeetingService = nullptr;

// Signal handler for clean shutdown
void signalHandler(int signal) {
    std::cout << "\n\n[SHUTDOWN] Received signal " << signal << " (";
    switch(signal) {
        case SIGINT: std::cout << "SIGINT - Ctrl+C"; break;
        case SIGTERM: std::cout << "SIGTERM - Termination"; break;
        default: std::cout << "Unknown"; break;
    }
    std::cout << "). Initiating clean shutdown..." << std::endl;
    std::cout.flush(); // Ensure output is visible immediately
    
    std::cout << "[DEBUG] Setting shouldExit flag..." << std::endl;
    shouldExit.store(true);
    std::cout << "[DEBUG] shouldExit set to: " << shouldExit.load() << std::endl;
    std::cout.flush();
    
    // Stop recording if active
    if (globalAudioHandler) {
        std::cout << "[SHUTDOWN] Stopping raw recording..." << std::endl;
        std::cout.flush();
        globalAudioHandler->stopRecording();
        
        std::cout << "[SHUTDOWN] Unsubscribing from audio data..." << std::endl;
        std::cout.flush();
        globalAudioHandler->unsubscribe();
    } else {
        std::cout << "[DEBUG] globalAudioHandler is null" << std::endl;
        std::cout.flush();
    }
    
    // Leave meeting
    if (globalMeetingService) {
        std::cout << "[SHUTDOWN] Leaving meeting..." << std::endl;
        std::cout.flush();
        auto leaveResult = globalMeetingService->Leave(ZOOM_SDK_NAMESPACE::LEAVE_MEETING);
        if (leaveResult == ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS) {
            std::cout << "[SHUTDOWN] ✓ Left meeting successfully" << std::endl;
        } else {
            std::cout << "[SHUTDOWN] Failed to leave meeting, error: " << leaveResult << std::endl;
        }
        std::cout.flush();
    } else {
        std::cout << "[DEBUG] globalMeetingService is null" << std::endl;
        std::cout.flush();
    }
    
    std::cout << "[SHUTDOWN] Clean shutdown completed. shouldExit = " << shouldExit.load() << std::endl;
    std::cout.flush();
}

// Set up robust signal handling
bool setupSignalHandling() {
    // First, test if signal handling works at all
    std::cout << "[DEBUG] Setting up signal handlers..." << std::endl;
    
    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // Restart interrupted system calls
    
    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        std::cerr << "Failed to set up SIGINT handler" << std::endl;
        return false;
    }
    
    if (sigaction(SIGTERM, &sa, nullptr) == -1) {
        std::cerr << "Failed to set up SIGTERM handler" << std::endl;
        return false;
    }
    
    std::cout << "[DEBUG] Signal handlers installed successfully" << std::endl;
    return true;
}

namespace {
    // Meeting timeout configuration
    constexpr int MEETING_TIMEOUT_SECONDS = 120;
}

static bool isVoipJoined(ZOOM_SDK_NAMESPACE::IMeetingService* meetingService) {
    if (!meetingService) return false;
    auto* pc = meetingService->GetMeetingParticipantsController();
    if (!pc) return false;
    auto* self = pc->GetMySelfUser();
    if (!self) return false;
    auto at = self->GetAudioJoinType();
    return at == ZOOM_SDK_NAMESPACE::AUDIOTYPE_VOIP || at == ZOOM_SDK_NAMESPACE::AUDIOTYPE_PHONE;
}

static bool waitForVoipJoin(ZOOM_SDK_NAMESPACE::IMeetingService* meetingService, int timeoutSeconds) {
    const int intervalMs = 500;
    int waitedMs = 0;
    std::cout << "[DEBUG] Waiting for VoIP join..." << std::endl;
    while (waitedMs < timeoutSeconds * 1000) {
        // Check for exit signal
        if (shouldExit.load()) {
            std::cout << "[DEBUG] Exit signal received during VoIP wait, aborting..." << std::endl;
            return false;
        }
        
        if (isVoipJoined(meetingService)) {
            std::cout << "Joined VoIP successfully" << std::endl;
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
        waitedMs += intervalMs;
    }
    std::cerr << "Timeout waiting for VoIP join" << std::endl;
    return false;
}

bool joinMeeting(ZOOM_SDK_NAMESPACE::IMeetingService* meetingService, 
                 MeetingEventHandler* eventHandler,
                 GMainLoop* mainLoop) {
    
    // Set event handler
    if (meetingService->SetEvent(eventHandler) != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS) {
        std::cerr << "Failed to set meeting event handler" << std::endl;
        return false;
    }

    // Prepare join parameters
    ZOOM_SDK_NAMESPACE::JoinParam joinParam;
    joinParam.userType = ZOOM_SDK_NAMESPACE::SDK_UT_WITHOUT_LOGIN;
    
    auto& normalUserParam = joinParam.param.withoutloginuserJoin;  // Use withoutloginJoin    

    // Clear all parameters first
    memset(&normalUserParam, 0, sizeof(normalUserParam));
    
    normalUserParam.meetingNumber = Config::getMeetingNumber();
    normalUserParam.userName = Config::getBotUsername().c_str();
    normalUserParam.psw = Config::getMeetingPassword().c_str();
    // Join meeting with audio so we can receive raw audio when licensed
    normalUserParam.isAudioOff = false;
    normalUserParam.isVideoOff = true;
    joinParam.param.withoutloginuserJoin = normalUserParam;

    std::cout << "\nJoin Parameters:" << std::endl;
    std::cout << "User Type: SDK_UT_WITHOUT_LOGIN" << std::endl;
    std::cout << "Meeting Number: " << normalUserParam.meetingNumber << std::endl;
    std::cout << "Username: " << normalUserParam.userName << std::endl;
    std::cout << "Password: " << normalUserParam.psw << std::endl;
    std::cout << "Not using ZAK token (not needed for participant join)" << std::endl;

    std::cout << "\nValidating join parameters..." << std::endl;
    if (normalUserParam.meetingNumber == 0 || !normalUserParam.userName || !normalUserParam.psw) {
        std::cerr << "✗ Missing required parameters" << std::endl;
        return false;
    }
    std::cout << "✓ All required parameters are present" << std::endl;

    std::cout << "\nAttempting to join meeting..." << std::endl;
    std::cout << "Meeting status before join: " << meetingService->GetMeetingStatus() << std::endl;
    
    auto joinResult = meetingService->Join(joinParam);
    std::cout << "\nJoin result code: " << joinResult;
    
    // Decode the join result
    switch(joinResult) {
        case ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS:
            std::cout << " (SUCCESS - join request accepted)"; break;
        case ZOOM_SDK_NAMESPACE::SDKERR_WRONG_USAGE:
            std::cout << " (WRONG_USAGE - incorrect parameters)"; break;
        case ZOOM_SDK_NAMESPACE::SDKERR_INVALID_PARAMETER:
            std::cout << " (INVALID_PARAMETER - bad parameters)"; break;
        case ZOOM_SDK_NAMESPACE::SDKERR_NO_IMPL:
            std::cout << " (NO_IMPL - not implemented)"; break;
        case ZOOM_SDK_NAMESPACE::SDKERR_UNINITIALIZE:
            std::cout << " (UNINITIALIZE - SDK not initialized)"; break;
        default:
            std::cout << " (UNKNOWN ERROR: " << joinResult << ")"; break;
    }
    std::cout << std::endl;
    
    std::cout << "Meeting status after join attempt: " << meetingService->GetMeetingStatus() << std::endl;
    
    if (joinResult != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS) {
        std::cerr << "Join meeting failed with SDK error code: " << joinResult << std::endl;
        return false;
    }

    return true;
}

bool waitForMeetingConnection(ZOOM_SDK_NAMESPACE::IMeetingService* meetingService,
                             MeetingEventHandler* eventHandler,
                             GMainLoop* mainLoop) {
    
    std::cout << "Join request sent! Waiting for meeting events..." << std::endl;

    // Add callback test to verify GMainLoop is working
    guint callbackTestId = g_timeout_add_seconds(10, [](gpointer data) -> gboolean {
        std::cout << "[CALLBACK TEST] GMainLoop is processing callbacks correctly" << std::endl;
        return TRUE;
    }, nullptr);

    // Create timeout for meeting join
    static bool timeoutTriggered = false;
    timeoutTriggered = false;
    
    guint meetingTimeoutId = g_timeout_add_seconds(MEETING_TIMEOUT_SECONDS, [](gpointer data) -> gboolean {
        std::cout << "Meeting join timeout reached (" << MEETING_TIMEOUT_SECONDS/60 << " minutes)" << std::endl;
        timeoutTriggered = true;
        auto* loop = static_cast<GMainLoop*>(data);
        g_main_loop_quit(loop);
        return FALSE;
    }, mainLoop);

    // Setup enhanced meeting detection
    guint statusUpdateId = MeetingDetector::setupActiveDetection(meetingService, eventHandler, mainLoop);

    std::cout << "Waiting up to " << MEETING_TIMEOUT_SECONDS/60 << " minutes for meeting connection..." << std::endl;
    std::cout << "(Active status checking every 5 seconds + callback monitoring)" << std::endl;
    std::cout << "This will detect meeting join even if callbacks aren't working properly..." << std::endl;
    
    // Wait for meeting events using GMainLoop
    g_main_loop_run(mainLoop);
    
    // Clean up all timers
    if (callbackTestId > 0) {
        g_source_remove(callbackTestId);
    }
    if (statusUpdateId > 0) {
        g_source_remove(statusUpdateId);
    }
    if (!timeoutTriggered && meetingTimeoutId > 0) {
        g_source_remove(meetingTimeoutId);
    }

    std::cout << "\nAnalyzing meeting join results..." << std::endl;

    // Check meeting results
    if (eventHandler->meetingFailed) {
        std::cerr << "Failed to join the meeting" << std::endl;
        return false;
    }
    
    if (!eventHandler->meetingJoined) {
        auto currentStatus = meetingService->GetMeetingStatus();
        std::cerr << "Timeout waiting for meeting join. Final status: " << currentStatus;
        switch(currentStatus) {
            case ZOOM_SDK_NAMESPACE::MEETING_STATUS_CONNECTING:
                std::cerr << " (Still connecting - check network/credentials)"; 
                std::cerr << "\nPossible issues:";
                std::cerr << "\n- Network connectivity problems";
                std::cerr << "\n- Incorrect meeting password";
                std::cerr << "\n- Meeting requires host approval";
                std::cerr << "\n- Meeting may be waiting for host to start";
                break;
            case ZOOM_SDK_NAMESPACE::MEETING_STATUS_WAITINGFORHOST:
                std::cerr << " (Waiting for host to start meeting)"; break;
            default:
                std::cerr << " (Unknown status)"; break;
        }
        std::cerr << std::endl;
        return false;
    }

    return true;
}

int main() {
    // Set up robust signal handling for graceful shutdown
    if (!setupSignalHandling()) {
        std::cerr << "Failed to set up signal handling" << std::endl;
        return -1;
    }
    std::cout << "Registered signal handlers for graceful shutdown (Ctrl+C)" << std::endl;

    // Initialize GMainLoop for SDK callbacks
    GMainLoop* mainLoop = g_main_loop_new(nullptr, FALSE);
    if (!mainLoop) {
        std::cerr << "Failed to create GMainLoop" << std::endl;
        return -1;
    }
    std::cout << "Created GMainLoop for handling SDK callbacks" << std::endl;
    // Log SDK version for diagnostics
    std::cout << "Zoom SDK Version: " << ZOOM_SDK_NAMESPACE::GetSDKVersion() << std::endl;

    // Step 1: Load configuration from environment variables
    if (!Config::loadFromEnvironment()) {
        std::cerr << "\n❌ Configuration Error: Missing required environment variables." << std::endl;
        std::cerr << "\nPlease set the following environment variables:" << std::endl;
        std::cerr << "  ZOOM_CLIENT_ID=your_client_id" << std::endl;
        std::cerr << "  ZOOM_CLIENT_SECRET=your_client_secret" << std::endl;
        std::cerr << "  ZOOM_ACCOUNT_ID=your_account_id" << std::endl;
        std::cerr << "  ZOOM_APP_KEY=your_app_key" << std::endl;
        std::cerr << "  ZOOM_APP_SECRET=your_app_secret" << std::endl;
        std::cerr << "  ZOOM_MEETING_NUMBER=meeting_id" << std::endl;
        std::cerr << "  ZOOM_MEETING_PASSWORD=meeting_password" << std::endl;
        std::cerr << "  ZOOM_BOT_USERNAME=bot_name (optional, defaults to 'ZoomBot')" << std::endl;
        std::cerr << "\nExample:" << std::endl;
        std::cerr << "  export ZOOM_CLIENT_ID=your_client_id" << std::endl;
        std::cerr << "  export ZOOM_CLIENT_SECRET=your_client_secret" << std::endl;
        std::cerr << "  # ... set other variables ..." << std::endl;
        std::cerr << "  ./zoom_poc" << std::endl;
        Config::printStatus();
        g_main_loop_unref(mainLoop);
        return -1;
    }
    
    Config::printStatus();

    // Step 2: Get OAuth and JWT tokens using configuration
    std::string oauthToken, jwtToken;
    
    // Get OAuth token using configuration
    oauthToken = getZoomAccessToken(Config::getClientId(), Config::getClientSecret(), Config::getAccountId());
    if (oauthToken.empty()) {
        std::cerr << "Failed to get OAuth token. Please check your OAuth credentials." << std::endl;
        g_main_loop_unref(mainLoop);
        return -1;
    }
    std::cout << "✅ Got OAuth token: " << oauthToken.substr(0, 20) << "..." << std::endl;
    std::cout << "Skipping ZAK token (not needed for participant join)" << std::endl;
    
    // Verify meeting exists
    std::cout << "\nMeeting number details:" << std::endl;
    std::cout << "Original format: " << Config::getMeetingNumber() << std::endl;
    std::cout << "Hex format: 0x" << std::hex << Config::getMeetingNumber() << std::dec << std::endl;
    
    if (!checkMeetingExists(oauthToken, Config::getMeetingNumber())) {
        std::cerr << "Meeting verification failed. Please check your meeting number." << std::endl;
        g_main_loop_unref(mainLoop);
        return -1;
    }
    std::cout << "✅ Meeting " << Config::getMeetingNumber() << " exists and is accessible" << std::endl;

    // Generate JWT token for SDK authentication using configuration
    nlohmann::json header = {
        {"alg", "HS256"},
        {"typ", "JWT"}
    };
    
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Convert meeting number to string without UL suffix
    std::ostringstream oss;
    oss << Config::getMeetingNumber();
    std::string meetingNumberStr = oss.str();
    
    nlohmann::json payload = {
        {"appKey", Config::getAppKey()},
        {"exp", now + 3600},
        {"iat", now},
        {"mn", meetingNumberStr},
        {"role", 0},
        {"sdkKey", Config::getAppKey()},
        {"tokenExp", now + 3600}
    };
    
    std::cout << "JWT payload: " << payload << std::endl;

    jwtToken = generateJWTToken(header, payload, Config::getAppSecret());
    if (jwtToken.empty()) {
        std::cerr << "Failed to generate JWT token. Please check your app credentials." << std::endl;
        g_main_loop_unref(mainLoop);
        return -1;
    }

    // Step 2: Initialize SDK
    auto initResult = SDKInitializer::initializeSDK();
    if (!initResult.success) {
        std::cerr << initResult.errorMessage << std::endl;
        g_main_loop_unref(mainLoop);
        return -1;
    }

    // Step 3: Authenticate SDK
    AuthEventHandler authHandler(mainLoop);
    if (!SDKInitializer::authenticateSDK(initResult.authService, &authHandler, mainLoop, jwtToken)) {
        SDKInitializer::cleanup(initResult);
        g_main_loop_unref(mainLoop);
        return -1;
    }

    // Step 4: Join meeting
    MeetingEventHandler meetingEventHandler(mainLoop);
    if (!joinMeeting(initResult.meetingService, &meetingEventHandler, mainLoop)) {
        SDKInitializer::cleanup(initResult);
        g_main_loop_unref(mainLoop);
        return -1;
    }

    // Step 5: Wait for meeting connection with enhanced detection
    if (!waitForMeetingConnection(initResult.meetingService, &meetingEventHandler, mainLoop)) {
        SDKInitializer::cleanup(initResult);
        g_main_loop_unref(mainLoop);
        return -1;
    }

    std::cout << "Successfully joined the meeting!" << std::endl;

    // Register recording event handler for raw data permission callbacks
    if (initResult.meetingService && initResult.meetingService->GetMeetingRecordingController()) {
        auto* recordingCtrl = initResult.meetingService->GetMeetingRecordingController();
        auto result = recordingCtrl->SetEvent(&meetingEventHandler);
        if (result == ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS) {
            std::cout << "Recording event handler registered for raw data permission tracking" << std::endl;
        } else {
            std::cerr << "Warning: Failed to register recording event handler: " << result << std::endl;
        }
    }

    // Step 6: Subscribe to raw audio and keep running
    // Join VoIP so we actually receive audio streams
    if (initResult.meetingService && initResult.meetingService->GetMeetingAudioController()) {
        auto* audioCtrl = initResult.meetingService->GetMeetingAudioController();
        // Optional: don't play audio locally to avoid feedback in headless env
        audioCtrl->EnablePlayMeetingAudio(false);
        auto je = audioCtrl->JoinVoip();
        std::cout << "JoinVoip result: " << je << std::endl;
    }

    ZoomBot::AudioRawHandler audioHandler;
    audioHandler.setMeetingService(initResult.meetingService);
    
    // Set global pointers for signal handler cleanup
    globalAudioHandler = &audioHandler;
    globalMeetingService = initResult.meetingService;
    std::cout << "[DEBUG] Global pointers set - audioHandler: " << globalAudioHandler 
              << ", meetingService: " << globalMeetingService << std::endl;
    
    // First request recording permission from host
    bool canProceedWithRecording = false;
    if (!audioHandler.requestRecordingPermission()) {
        std::cerr << "Warning: Could not request recording permission from host." << std::endl;
        std::cerr << "This may be because the meeting doesn't support recording requests," << std::endl;
        std::cerr << "or the bot doesn't have permission to request recording." << std::endl;
        std::cerr << "Will attempt raw audio subscription without explicit permission." << std::endl;
        // Allow proceeding without explicit permission in case raw data is available anyway
        canProceedWithRecording = true;
    } else {
        std::cout << "Recording permission requested. Waiting for host approval..." << std::endl;
        // Wait up to 30 seconds for host response
        int waitSeconds = 30;
        int waited = 0;
        while (waited < waitSeconds && !meetingEventHandler.recordingPermissionGranted) {
            // Check for exit signal
            if (shouldExit.load()) {
                std::cout << "[DEBUG] Exit signal received during recording permission wait, aborting..." << std::endl;
                return -1;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
            waited++;
            // Allow GLib to process callbacks
            while (g_main_context_iteration(nullptr, FALSE)) {}
        }
        
        if (meetingEventHandler.recordingPermissionGranted) {
            std::cout << "Host granted recording permission! Starting raw recording..." << std::endl;
            // Give a moment for the permission to propagate through the SDK
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // Start raw recording - this should enable raw data access
            bool recordingStarted = audioHandler.startRecording();
            if (recordingStarted) {
                std::cout << "Raw recording started successfully. Raw data access should now be available." << std::endl;
                // Give recording a moment to initialize
                std::this_thread::sleep_for(std::chrono::seconds(2));
                canProceedWithRecording = true;
            } else {
                std::cerr << "Failed to start raw recording despite having permission." << std::endl;
                std::cerr << "Will still attempt raw audio subscription in case it works anyway." << std::endl;
                canProceedWithRecording = true;
            }
        } else if (meetingEventHandler.recordingPermissionDenied) {
            std::cerr << "Host explicitly DENIED recording permission." << std::endl;
            std::cerr << "Bot will respect the host's decision and will NOT attempt to record." << std::endl;
            canProceedWithRecording = false;
        } else {
            std::cerr << "Host did not respond to recording permission request (timeout)." << std::endl;
            std::cerr << "This could mean:" << std::endl;
            std::cerr << "1. Host didn't see or respond to the request" << std::endl;
            std::cerr << "2. Host is not present to approve the request" << std::endl;
            std::cerr << "3. Meeting settings don't require explicit approval" << std::endl;
            std::cerr << "Bot will attempt recording in case it's automatically allowed." << std::endl;
            canProceedWithRecording = true;
        }
    }
    
    // Wait up to 20s for VOIP to actually join before subscribing
    if (!waitForVoipJoin(initResult.meetingService, 20)) {
        std::cerr << "VoIP join failed or timed out." << std::endl;
        if (!canProceedWithRecording) {
            std::cerr << "Cannot proceed with recording without VoIP and without recording permission." << std::endl;
        }
    }
    
    bool audioSubscribed = false;
    if (canProceedWithRecording) {
        std::cout << "\n[AUDIO] Attempting to subscribe to raw audio data..." << std::endl;
        audioSubscribed = audioHandler.subscribe(false);
        if (!audioSubscribed) {
            std::cout << "\n=== AUDIO SUBSCRIPTION FAILED ===" << std::endl;
            std::cout << "Raw audio capture could not be enabled. This is likely because:" << std::endl;
            std::cout << "1. Host has not granted recording permission" << std::endl;
            std::cout << "2. Meeting doesn't support raw audio capture" << std::endl;
            std::cout << "3. Audio raw data helper is not available" << std::endl;
            std::cout << "\nThe bot will remain in the meeting but won't capture audio." << std::endl;
            std::cout << "=========================================" << std::endl;
        } else {
            std::cout << "\n[AUDIO] ✓ Raw audio subscription successful!" << std::endl;
            std::cout << "[AUDIO] Starting per-participant audio capture..." << std::endl;
            
            // Enable audio streaming to Python processing service
            std::cout << "[STREAMING] Enabling audio streaming..." << std::endl;
            bool streamingEnabled = audioHandler.enableStreaming("tcp", "localhost:8888");
            if (streamingEnabled) {
                std::cout << "[STREAMING] ✓ Audio streaming enabled!" << std::endl;
                std::cout << "[STREAMING] Audio will be streamed to Python service for processing" << std::endl;
            } else {
                std::cout << "[STREAMING] ⚠ Audio streaming failed - continuing with file recording only" << std::endl;
            }
        }
    } else {
        std::cout << "\n=== RECORDING PERMISSION DENIED ===" << std::endl;
        std::cout << "Bot will NOT attempt to subscribe to raw audio data because:" << std::endl;
        std::cout << "- Recording permission was not granted by the host" << std::endl;
        std::cout << "- Respecting meeting privacy and host's decision" << std::endl;
        std::cout << "\nThe bot will remain in the meeting but will NOT capture audio." << std::endl;
        std::cout << "To record audio, the host must explicitly grant recording permission." << std::endl;
        std::cout << "=======================================" << std::endl;
    }
    
    if (audioSubscribed) {
        std::cout << "\nBot is now in the meeting and recording per-participant PCM in ./recordings." << std::endl;
        std::cout << "Press Ctrl+C to exit, or type 'quit' + Enter if Ctrl+C doesn't work..." << std::endl;
        std::cout << "Note: When you stop recording, all PCM files will be automatically converted to WAV format for easy playback." << std::endl;
    } else {
        std::cout << "\nBot is now in the meeting but is NOT recording audio." << std::endl;
        std::cout << "Press Ctrl+C to exit, or type 'quit' + Enter if Ctrl+C doesn't work..." << std::endl;
        if (!canProceedWithRecording) {
            std::cout << "Recording was disabled because the host did not grant recording permission." << std::endl;
        }
    }
    std::cout << "[DEBUG] About to enter main loop, shouldExit = " << shouldExit.load() << std::endl;
    std::cout.flush();
    
    int loopCount = 0;
    std::cout << "[DEBUG] Entering main loop now..." << std::endl;
    std::cout.flush();
    
    // Keep running as long as our enhanced detection considered us joined and no exit signal received
    while (true) {
        // Check exit condition first - this is our primary exit mechanism
        if (shouldExit.load()) {
            std::cout << "\n[SHUTDOWN] Exit signal detected, breaking main loop..." << std::endl;
            std::cout.flush();
            break;
        }
        
        // Check if meeting is still active
        if (!meetingEventHandler.meetingJoined) {
            std::cout << "\n[MEETING] Meeting left detected, breaking main loop..." << std::endl;
            std::cout.flush();
            break;
        }
        
        // Process GLib events with timeout to ensure responsiveness
        g_main_context_iteration(nullptr, FALSE);
        
        // Sleep for a short time to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Debug output every 50 iterations (5 seconds)
        loopCount++;
        if (loopCount % 50 == 0) {
            std::cout << "[DEBUG] Main loop running, shouldExit = " << shouldExit.load() 
                      << ", meetingJoined = " << meetingEventHandler.meetingJoined << std::endl;
            std::cout.flush();
        }
        
        // Check meeting status
        auto st = initResult.meetingService->GetMeetingStatus();
        if (st == ZOOM_SDK_NAMESPACE::MEETING_STATUS_FAILED || st == ZOOM_SDK_NAMESPACE::MEETING_STATUS_IDLE) {
            std::cout << "\n[MEETING] Meeting ended (status: " << st << "), exiting..." << std::endl;
            break;
        }
    }
    
    // Check if we're exiting due to signal
    if (shouldExit.load()) {
        std::cout << "\n[SHUTDOWN] Graceful shutdown initiated..." << std::endl;
    }

    // Unsubscribe before leaving
    audioHandler.unsubscribe();
    
    // Clear global pointers
    globalAudioHandler = nullptr;
    globalMeetingService = nullptr;

    // Cleanup
    SDKInitializer::cleanup(initResult);
    g_main_loop_unref(mainLoop);
    
    return 0;
}