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
#include <algorithm>
#include <cctype>

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
#include "audio_raw_handler.h"
#include "config.h"
#include "token_manager.h"
#include "meeting_setup.h"
#include "audio_manager.h"

using namespace ZoomBot;

// Global variables for clean shutdown
std::atomic<bool> shouldExit{false};
ZoomBot::AudioRawHandler* globalAudioHandler = nullptr;
ZOOM_SDK_NAMESPACE::IMeetingService* globalMeetingService = nullptr;

// Signal handler for clean shutdown
void signalHandler(int signal) {
    std::cout << "\n[SHUTDOWN] Received signal " << signal << " - initiating clean shutdown..." << std::endl;
    std::cout.flush();
    
    shouldExit.store(true);
    
    // Stop recording if active
    if (globalAudioHandler) {
        std::cout << "[SHUTDOWN] Stopping audio recording..." << std::endl;
        globalAudioHandler->stopRecording();
        globalAudioHandler->unsubscribe();
    }
    
    // Leave meeting
    if (globalMeetingService) {
        std::cout << "[SHUTDOWN] Leaving meeting..." << std::endl;
        auto leaveResult = globalMeetingService->Leave(ZOOM_SDK_NAMESPACE::LEAVE_MEETING);
        if (leaveResult == ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS) {
            std::cout << "[SHUTDOWN] ✓ Left meeting" << std::endl;
        }
    }
    
    std::cout << "[SHUTDOWN] Shutdown complete" << std::endl;
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
    
    // Set event handlers
    if (meetingService->SetEvent(eventHandler) != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS) {
        std::cerr << "Failed to set meeting event handler" << std::endl;
        return false;
    }
    
    // Register recording event handler
    auto* recordingController = meetingService->GetMeetingRecordingController();
    if (recordingController) {
        if (recordingController->SetEvent(eventHandler) == ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS) {
            std::cout << "✓ Recording event handler registered" << std::endl;
        } else {
            std::cerr << "⚠ Failed to register recording event handler" << std::endl;
        }
    } else {
        std::cerr << "⚠ Recording controller not available during join" << std::endl;
    }
    
    // TODO: Register waiting room event handler (requires separate handler class)
    // Waiting room functionality handled via meeting status callbacks for now
    std::cout << "⚠ Waiting room events handled via meeting status (full handler not yet implemented)" << std::endl;

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
        
        // Special handling for waiting room - this is actually a successful connection
        if (currentStatus == ZOOM_SDK_NAMESPACE::MEETING_STATUS_IN_WAITING_ROOM || eventHandler->inWaitingRoom) {
            std::cout << "✓ Bot successfully connected and is in the waiting room!" << std::endl;
            std::cout << "  Waiting for host to admit the bot into the meeting..." << std::endl;
            std::cout << "  This may take a while - the bot will wait until admitted." << std::endl;
            
            // Continue waiting for admission from waiting room
            return true; // Treat waiting room as successful connection
        }
        
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
                std::cerr << " (Waiting for host to start meeting)"; 
                // This is also a successful connection, just waiting for host
                std::cout << "\n✓ Bot connected and waiting for host to start meeting" << std::endl;
                return true;
            case ZOOM_SDK_NAMESPACE::MEETING_STATUS_IN_WAITING_ROOM:
                std::cout << "\n✓ Bot is in waiting room - this is a successful connection!" << std::endl;
                return true;
            default:
                std::cerr << " (Unknown status)"; break;
        }
        std::cerr << std::endl;
        return false;
    }

    return true;
}

// Function declarations
bool setupEnvironmentAndCredentials();
bool getMeetingDetailsFromUser();
bool authenticateWithZoom();
bool initializeSDKAndJoinMeeting(GMainLoop* mainLoop, ZoomBot::SDKInitializer::InitResult& initResult, MeetingEventHandler& eventHandler);
bool setupAudioRecording(ZOOM_SDK_NAMESPACE::IMeetingService* meetingService, ZoomBot::AudioRawHandler& audioHandler, MeetingEventHandler& eventHandler);
void runMeetingLoop(ZOOM_SDK_NAMESPACE::IMeetingService* meetingService, MeetingEventHandler& eventHandler, ZoomBot::AudioRawHandler& audioHandler);

// Helper function to trim whitespace from a string
std::string trim(const std::string& str) {
    auto start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    auto end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

// Helper function to remove spaces from meeting number and validate format
std::string parseMeetingNumber(const std::string& input) {
    std::string trimmed = trim(input);
    std::string cleaned;
    
    // Remove all spaces
    for (char c : trimmed) {
        if (c != ' ') {
            cleaned += c;
        }
    }
    
    // Validate that it contains only digits and is 11 characters long
    if (cleaned.length() != 11) {
        return "";
    }
    
    for (char c : cleaned) {
        if (!std::isdigit(c)) {
            return "";
        }
    }
    
    return cleaned;
}

// Function to get meeting details from console input
bool getMeetingDetailsFromConsole(std::string& meetingNumber, std::string& meetingPassword) {
    std::cout << "\n🎥 Zoom Bot Meeting Setup" << std::endl;
    std::cout << "=========================" << std::endl;
    
    // Get meeting number
    std::cout << "\nEnter meeting number (format: XXX XXXX XXXX): ";
    std::string meetingInput;
    std::getline(std::cin, meetingInput);
    
    meetingNumber = parseMeetingNumber(meetingInput);
    if (meetingNumber.empty()) {
        std::cerr << "❌ Invalid meeting number format!" << std::endl;
        std::cerr << "Expected format: XXX XXXX XXXX (11 digits with spaces)" << std::endl;
        std::cerr << "Example: 123 4567 8901" << std::endl;
        return false;
    }
    
    std::cout << "✅ Meeting number parsed: " << meetingNumber << std::endl;
    
    // Get meeting password
    std::cout << "Enter meeting password: ";
    std::getline(std::cin, meetingPassword);
    meetingPassword = trim(meetingPassword);
    
    if (meetingPassword.empty()) {
        std::cerr << "❌ Meeting password cannot be empty!" << std::endl;
        return false;
    }
    
    std::cout << "✅ Meeting password entered" << std::endl;
    
    // Confirmation
    std::cout << "\n📋 Meeting Details Summary:" << std::endl;
    std::cout << "  Meeting Number: " << meetingNumber << std::endl;
    std::cout << "  Password: " << std::string(meetingPassword.length(), '*') << std::endl;
    std::cout << "\nProceed with these details? (y/N): ";
    
    std::string confirm;
    std::getline(std::cin, confirm);
    confirm = trim(confirm);
    std::transform(confirm.begin(), confirm.end(), confirm.begin(), ::tolower);
    
    if (confirm != "y" && confirm != "yes") {
        std::cout << "❌ Meeting setup cancelled." << std::endl;
        return false;
    }
    
    std::cout << "✅ Meeting details confirmed!" << std::endl;
    return true;
}

int main() {
    // Set up signal handling
    if (!setupSignalHandling()) {
        std::cerr << "Failed to set up signal handling" << std::endl;
        return -1;
    }
    std::cout << "✓ Signal handlers registered" << std::endl;

    // Initialize GMainLoop
    GMainLoop* mainLoop = g_main_loop_new(nullptr, FALSE);
    if (!mainLoop) {
        std::cerr << "Failed to create GMainLoop" << std::endl;
        return -1;
    }
    std::cout << "✓ GMainLoop initialized" << std::endl;
    
    std::cout << "Zoom SDK Version: " << ZOOM_SDK_NAMESPACE::GetSDKVersion() << std::endl;

    // Step 1: Setup environment and credentials
    if (!setupEnvironmentAndCredentials()) {
        g_main_loop_unref(mainLoop);
        return -1;
    }

    // Step 2: Get meeting details from user
    if (!getMeetingDetailsFromUser()) {
        g_main_loop_unref(mainLoop);
        return -1;
    }

    // Step 3: Authenticate with Zoom
    if (!authenticateWithZoom()) {
        g_main_loop_unref(mainLoop);
        return -1;
    }

    // Step 4: Initialize SDK and join meeting
    ZoomBot::SDKInitializer::InitResult initResult;
    MeetingEventHandler eventHandler(mainLoop);
    
    if (!initializeSDKAndJoinMeeting(mainLoop, initResult, eventHandler)) {
        g_main_loop_unref(mainLoop);
        return -1;
    }

    std::cout << "✓ Successfully joined the meeting!" << std::endl;

    // Step 5: Setup audio recording (only if not in waiting room)
    ZoomBot::AudioRawHandler audioHandler;
    globalAudioHandler = &audioHandler;
    globalMeetingService = initResult.meetingService;

    // Check if bot is in waiting room - if so, delay audio setup until admission
    auto currentStatus = initResult.meetingService->GetMeetingStatus();
    if (currentStatus == ZOOM_SDK_NAMESPACE::MEETING_STATUS_IN_WAITING_ROOM || eventHandler.inWaitingRoom) {
        std::cout << "\n[WAITING ROOM] Bot is in waiting room - audio setup will be done after host admits bot" << std::endl;
    } else {
        std::cout << "\n[AUDIO] Setting up audio recording..." << std::endl;
        if (!setupAudioRecording(initResult.meetingService, audioHandler, eventHandler)) {
            std::cout << "⚠ Audio recording setup failed - continuing without recording" << std::endl;
        }
    }

    // Step 6: Run the meeting loop
    std::cout << "\nBot is active. Press Ctrl+C to exit..." << std::endl;
    runMeetingLoop(initResult.meetingService, eventHandler, audioHandler);

    // Cleanup
    audioHandler.unsubscribe();
    globalAudioHandler = nullptr;
    globalMeetingService = nullptr;
    SDKInitializer::cleanup(initResult);
    g_main_loop_unref(mainLoop);
    
    return 0;
}

bool setupEnvironmentAndCredentials() {
    Config::loadFromEnvironment();
    
    if (!Config::areCredentialsValid()) {
        std::cerr << "\n❌ Missing Zoom credentials. Please set:" << std::endl;
        std::cerr << "  ZOOM_CLIENT_ID, ZOOM_CLIENT_SECRET, ZOOM_ACCOUNT_ID" << std::endl;
        std::cerr << "  ZOOM_APP_KEY, ZOOM_APP_SECRET" << std::endl;
        return false;
    }
    
    std::cout << "✓ Credentials loaded" << std::endl;
    return true;
}

bool getMeetingDetailsFromUser() {
    auto meetingDetails = ZoomBot::MeetingSetup::getMeetingDetailsFromConsole();
    
    if (!meetingDetails.success) {
        std::cerr << "❌ " << meetingDetails.errorMessage << std::endl;
        return false;
    }

    try {
        uint64_t meetingNumber = std::stoull(meetingDetails.meetingNumber);
        Config::setMeetingNumber(meetingNumber);
        Config::setMeetingPassword(meetingDetails.password);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "❌ Error parsing meeting number: " << e.what() << std::endl;
        return false;
    }
}

bool authenticateWithZoom() {
    // Get OAuth token
    auto oauthResult = ZoomBot::TokenManager::getOAuthToken(
        Config::getClientId(), 
        Config::getClientSecret(), 
        Config::getAccountId()
    );
    
    if (!oauthResult.success) {
        std::cerr << "❌ " << oauthResult.errorMessage << std::endl;
        return false;
    }

    // Verify meeting exists
    if (!ZoomBot::TokenManager::verifyMeetingExists(oauthResult.token, Config::getMeetingNumber())) {
        return false;
    }

    // Generate JWT token
    auto jwtResult = ZoomBot::TokenManager::generateJWTToken(
        Config::getAppKey(),
        Config::getAppSecret(),
        Config::getMeetingNumber()
    );
    
    if (!jwtResult.success) {
        std::cerr << "❌ " << jwtResult.errorMessage << std::endl;
        return false;
    }

    // Store JWT token for SDK authentication
    Config::setJWTToken(jwtResult.token);
    return true;
}

bool initializeSDKAndJoinMeeting(GMainLoop* mainLoop, ZoomBot::SDKInitializer::InitResult& initResult, MeetingEventHandler& eventHandler) {
    // Initialize SDK
    initResult = SDKInitializer::initializeSDK();
    if (!initResult.success) {
        std::cerr << "❌ " << initResult.errorMessage << std::endl;
        return false;
    }
    std::cout << "✓ SDK initialized" << std::endl;

    // Authenticate SDK
    AuthEventHandler authHandler(mainLoop);
    if (!SDKInitializer::authenticateSDK(initResult.authService, &authHandler, mainLoop, Config::getJWTToken())) {
        std::cerr << "❌ SDK authentication failed" << std::endl;
        return false;
    }
    std::cout << "✓ SDK authenticated" << std::endl;

    // Join meeting
    if (!joinMeeting(initResult.meetingService, &eventHandler, mainLoop)) {
        std::cerr << "❌ Failed to join meeting" << std::endl;
        return false;
    }

    // Wait for connection
    if (!waitForMeetingConnection(initResult.meetingService, &eventHandler, mainLoop)) {
        std::cerr << "❌ Meeting connection failed" << std::endl;
        return false;
    }

    return true;
}

bool setupAudioRecording(ZOOM_SDK_NAMESPACE::IMeetingService* meetingService, ZoomBot::AudioRawHandler& audioHandler, MeetingEventHandler& eventHandler) {
    // Check if we were admitted from waiting room - this affects timing
    if (eventHandler.admittedFromWaitingRoom) {
        std::cout << "[AUDIO] Bot was admitted from waiting room - using extended setup timing..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    } else {
        std::cout << "[AUDIO] Waiting 3 seconds for meeting to stabilize..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
    
    // Set meeting service for audio handler
    audioHandler.setMeetingService(meetingService);
    
    // Re-register recording event handler (may be needed after waiting room)
    auto* recordingController = meetingService->GetMeetingRecordingController();
    if (recordingController) {
        recordingController->SetEvent(&eventHandler);
        std::cout << "[RECORDING] Recording event handler re-registered after meeting join" << std::endl;
    }
    
    // Request host recording permission first (async - no waiting)
    std::cout << "[RECORDING] Requesting host to start recording..." << std::endl;
    if (audioHandler.requestRecordingPermission()) {
        std::cout << "✓ Recording permission requested from host" << std::endl;
        std::cout << "[RECORDING] Permission request sent - will retry audio setup when host responds" << std::endl;
        // Set flag to indicate we're waiting for permission
        eventHandler.needsAudioRetryAfterPermission = true;
    } else {
        std::cout << "⚠ Could not request recording permission - may not be needed" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    // Now attempt audio capture setup
    auto audioResult = ZoomBot::AudioManager::setupAudioCapture(meetingService, audioHandler);
    
    if (audioResult.success) {
        std::cout << "✓ " << audioResult.statusMessage << std::endl;
        if (audioResult.streamingEnabled) {
            std::cout << "✓ Audio streaming to Python service enabled" << std::endl;
        }
        std::cout << "\nRecording to: ./recordings/" << std::endl;
        return true;
    } else {
        std::cout << "✗ " << audioResult.statusMessage << std::endl;
        
        // If VoIP join failed, try once more after additional wait
        if (audioResult.statusMessage.find("VoIP join failed") != std::string::npos) {
            std::cout << "[AUDIO] Retrying VoIP join after additional wait..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            auto retryResult = ZoomBot::AudioManager::setupAudioCapture(meetingService, audioHandler);
            if (retryResult.success) {
                std::cout << "✓ " << retryResult.statusMessage << " (on retry)" << std::endl;
                return true;
            }
        }
        
        // If audio subscription failed due to permission, that's expected when waiting for host approval
        if (audioResult.statusMessage.find("no recording permission") != std::string::npos ||
            audioResult.statusMessage.find("NO_PERMISSION") != std::string::npos) {
            std::cout << "⚠ Audio setup will be retried when recording permission is granted by host" << std::endl;
            return true; // Return true so we don't fail completely - just wait for permission
        }
        
        return false;
    }
}

void runMeetingLoop(ZOOM_SDK_NAMESPACE::IMeetingService* meetingService, MeetingEventHandler& eventHandler, ZoomBot::AudioRawHandler& audioHandler) {
    int loopCount = 0;
    bool wasInWaitingRoom = eventHandler.inWaitingRoom;
    bool audioSetupCompleted = false;
    
    while (true) {
        if (shouldExit.load()) {
            std::cout << "\n[SHUTDOWN] Graceful shutdown initiated..." << std::endl;
            break;
        }
        
        auto currentStatus = meetingService->GetMeetingStatus();
        
        // Don't exit if we're in waiting room or waiting for host
        bool shouldContinue = eventHandler.meetingJoined || 
                             eventHandler.inWaitingRoom || 
                             currentStatus == ZOOM_SDK_NAMESPACE::MEETING_STATUS_IN_WAITING_ROOM ||
                             currentStatus == ZOOM_SDK_NAMESPACE::MEETING_STATUS_WAITINGFORHOST ||
                             currentStatus == ZOOM_SDK_NAMESPACE::MEETING_STATUS_CONNECTING;
        
        if (!shouldContinue) {
            std::cout << "\n[MEETING] Left meeting or disconnected" << std::endl;
            break;
        }
        
        // Check if bot was just admitted from waiting room - use the flag set by callback
        if (eventHandler.needsAudioSetupAfterAdmission && !audioSetupCompleted) {
            std::cout << "\n[WAITING ROOM] Bot was admitted to meeting! Setting up audio..." << std::endl;
            
            // Clear the flag first
            eventHandler.needsAudioSetupAfterAdmission = false;
            
            // Use the existing audio handler that was passed to the function
            if (setupAudioRecording(meetingService, audioHandler, eventHandler)) {
                std::cout << "✓ Audio recording setup completed after waiting room admission" << std::endl;
                audioSetupCompleted = true;
            } else {
                std::cout << "⚠ Audio recording setup failed after waiting room admission" << std::endl;
            }
        }
        
        // Check if recording permission was granted and we need to retry audio setup
        if (eventHandler.needsAudioRetryAfterPermission) {
            std::cout << "\n[PERMISSION] **DETECTED** Recording permission granted! Retrying audio setup..." << std::endl;
            
            // Clear the flag first
            eventHandler.needsAudioRetryAfterPermission = false;
            
            // Wait a moment for permission to propagate
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            // First, start raw recording (this may be required before subscription)
            std::cout << "[PERMISSION] Starting raw recording with granted permission..." << std::endl;
            if (audioHandler.startRecording()) {
                std::cout << "✓ Raw recording started successfully" << std::endl;
                
                // Now try to subscribe to audio data
                if (audioHandler.subscribe(false)) {
                    std::cout << "✓ Audio subscription successful after permission grant" << std::endl;
                    audioSetupCompleted = true;
                    
                    // Enable streaming if available
                    if (audioHandler.enableStreaming("tcp", "localhost:8888")) {
                        std::cout << "✓ Audio streaming enabled" << std::endl;
                    }
                } else {
                    std::cout << "⚠ Audio subscription failed even after starting recording" << std::endl;
                }
            } else {
                std::cout << "⚠ Could not start raw recording even with permission" << std::endl;
            }
        }
        
        // Process events
        g_main_context_iteration(nullptr, FALSE);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Periodic status (adjust message based on state)
        loopCount++;
        if (loopCount % 100 == 0) { // Every 10 seconds
            if (eventHandler.inWaitingRoom) {
                std::cout << "[STATUS] Bot in waiting room, waiting for host admission..." << std::endl;
            } else if (currentStatus == ZOOM_SDK_NAMESPACE::MEETING_STATUS_WAITINGFORHOST) {
                std::cout << "[STATUS] Bot connected, waiting for host to start meeting..." << std::endl;
            } else if (eventHandler.meetingJoined) {
                std::cout << "[STATUS] Bot active in meeting";
                if (eventHandler.needsAudioRetryAfterPermission) {
                    std::cout << " (waiting for recording permission)";
                } else if (audioSetupCompleted) {
                    std::cout << " (recording active)";
                } else {
                    std::cout << " (audio setup pending)";
                }
                std::cout << std::endl;
            } else {
                std::cout << "[STATUS] Bot connecting to meeting..." << std::endl;
            }
        }
        
        // Check for terminal meeting states
        if (currentStatus == ZOOM_SDK_NAMESPACE::MEETING_STATUS_FAILED || 
            currentStatus == ZOOM_SDK_NAMESPACE::MEETING_STATUS_IDLE ||
            currentStatus == ZOOM_SDK_NAMESPACE::MEETING_STATUS_ENDED) {
            std::cout << "\n[MEETING] Meeting ended or failed (status: " << currentStatus << ")" << std::endl;
            break;
        }
    }
}