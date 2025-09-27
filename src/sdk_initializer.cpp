#include "sdk_initializer.h"
#include "zoom_auth.h"
#include "jwt_helper.h"

namespace ZoomBot {

SDKInitializer::InitResult SDKInitializer::initializeSDK() {
    InitResult result;
    
    std::cout << "Initializing SDK..." << std::endl;
    
    // Setup environment and preload libraries
    setupEnvironment();
    preloadLibraries();
    
    // Initialize SDK
    auto initParam = createInitParams();
    if (ZOOM_SDK_NAMESPACE::InitSDK(initParam) != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS) {
        result.errorMessage = "Zoom SDK init failed";
        return result;
    }
    std::cout << "Zoom SDK initialized" << std::endl;
    
    // Create auth service
    if (ZOOM_SDK_NAMESPACE::CreateAuthService(&result.authService) != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS || !result.authService) {
        result.errorMessage = "Failed to create auth service";
        return result;
    }
    
    // Create meeting service
    if (ZOOM_SDK_NAMESPACE::CreateMeetingService(&result.meetingService) != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS || !result.meetingService) {
        result.errorMessage = "Failed to create meeting service";
        return result;
    }
    
    result.success = true;
    return result;
}

bool SDKInitializer::authenticateSDK(
    ZOOM_SDK_NAMESPACE::IAuthService* authService,
    AuthEventHandler* authHandler,
    GMainLoop* mainLoop,
    const std::string& jwtToken
) {
    // Set event handler
    auto setEventResult = authService->SetEvent(authHandler);
    if (setEventResult != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS) {
        std::cerr << "Failed to set auth event handler: " << setEventResult << std::endl;
        return false;
    }
    std::cout << "Auth event handler registered successfully" << std::endl;

    // Prepare auth context
    ZOOM_SDK_NAMESPACE::AuthContext authContext;
    authContext.jwt_token = jwtToken.c_str();
    
    std::cout << "\nValidating JWT components..." << std::endl;
    // Add basic validation
    if (jwtToken.empty()) {
        std::cerr << "JWT token is empty" << std::endl;
        return false;
    }
    std::cout << "Using JWT token for SDK authentication: " << jwtToken.substr(0, 100) << "..." << std::endl;
    std::cout << "Attempting SDK authentication..." << std::endl;

    // Check current auth status
    auto currentAuthResult = authService->GetAuthResult();
    std::cout << "Current auth status: " << currentAuthResult << std::endl;
    
    // Attempt authentication
    auto authResult = authService->SDKAuth(authContext);
    std::cout << "Auth request result: " << authResult << std::endl;
    
    if (authResult != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS) {
        std::cerr << "Failed to initiate SDK authentication" << std::endl;
        return false;
    }
    
    std::cout << "SDK authentication request sent, waiting for callback..." << std::endl;

    // Create timeout for authentication
    guint timeoutId = g_timeout_add_seconds(60, [](gpointer data) -> gboolean {
        GMainLoop* loop = static_cast<GMainLoop*>(data);
        std::cout << "Authentication timeout reached" << std::endl;
        g_main_loop_quit(loop);
        return FALSE;
    }, mainLoop);

    std::cout << "\nWaiting for authentication callback with GMainLoop..." << std::endl;
    
    // Run the main loop - this will process SDK callbacks
    g_main_loop_run(mainLoop);
    
    // Remove timeout if still active
    if (timeoutId > 0) {
        g_source_remove(timeoutId);
    }
    
    std::cout << "GMainLoop exited" << std::endl;

    // Check authentication results
    if (!authHandler->authenticationCompleted) {
        std::cerr << "SDK authentication timed out waiting for callback" << std::endl;
        return false;
    }

    if (authHandler->lastResult != ZOOM_SDK_NAMESPACE::AUTHRET_SUCCESS) {
        std::cerr << "SDK authentication failed with result: " << authHandler->lastResult << std::endl;
        return false;
    }

    std::cout << "SDK authenticated successfully!" << std::endl;
    return true;
}

void SDKInitializer::cleanup(const InitResult& result) {
    std::cout << "Cleaning up..." << std::endl;
    if (result.meetingService) {
        ZOOM_SDK_NAMESPACE::DestroyMeetingService(result.meetingService);
    }
    // Note: SDK cleanup function varies by version - omitting for safety
}

void SDKInitializer::setupEnvironment() {
    const char* sdkPath = "/workspaces/zoom-bot/zoom-sdk";
    setenv("LD_LIBRARY_PATH", sdkPath, 1);
    std::cout << "Using SDK path: " << sdkPath << std::endl;
    std::cout << "LD_LIBRARY_PATH: " << getenv("LD_LIBRARY_PATH") << std::endl;
}

void SDKInitializer::preloadLibraries() {
    std::string cmd = "cd /workspaces/zoom-bot/zoom-sdk && "
                     "ln -sf libmeetingsdk.so libmeeting_sdk_wrapper.so && "
                     "ln -sf libmeetingsdk.so libssb_sdk.so && "
                     "ls -la lib*";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            std::cout << buffer;
        }
        pclose(pipe);
    }
}

ZOOM_SDK_NAMESPACE::InitParam SDKInitializer::createInitParams() {
    ZOOM_SDK_NAMESPACE::InitParam initParam;
    initParam.strWebDomain = "https://zoom.us";
    initParam.emLanguageID = ZOOM_SDK_NAMESPACE::LANGUAGE_English;
    initParam.enableLogByDefault = true;
    initParam.enableGenerateDump = true;
    initParam.uiLogFileSize = 10;  // 10MB log file size
    // Prefer heap for raw audio buffers when we may retain frames briefly
    initParam.rawdataOpts.audioRawdataMemoryMode = ZOOM_SDK_NAMESPACE::ZoomSDKRawDataMemoryModeHeap;
    return initParam;
}

} // namespace ZoomBot