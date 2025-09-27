#include <iostream>
#include <glib.h>
#include "zoom_sdk.h"
#include "zoom_sdk_def.h"
#include "auth_service_interface.h"
#include "jwt_helper.h"
#include <nlohmann/json.hpp>

class TestAuthHandler : public ZOOM_SDK_NAMESPACE::IAuthServiceEvent {
public:
    bool authenticationCompleted = false;
    ZOOM_SDK_NAMESPACE::AuthResult lastResult = ZOOM_SDK_NAMESPACE::AUTHRET_NONE;
    GMainLoop* mainLoop = nullptr;

    TestAuthHandler(GMainLoop* loop) : mainLoop(loop) {}

    virtual void onAuthenticationReturn(ZOOM_SDK_NAMESPACE::AuthResult ret) override {
        std::cout << "\n[SUCCESS] Authentication callback received! Result: " << ret << std::endl;
        
        authenticationCompleted = true;
        lastResult = ret;
        
        std::cout << "Status: ";
        switch(ret) {
            case ZOOM_SDK_NAMESPACE::AUTHRET_SUCCESS:
                std::cout << "Authentication successful!"; break;
            case ZOOM_SDK_NAMESPACE::AUTHRET_KEYORSECRETEMPTY:
                std::cout << "Key or secret is empty"; break;
            case ZOOM_SDK_NAMESPACE::AUTHRET_KEYORSECRETWRONG:
                std::cout << "Key or secret is wrong"; break;
            case ZOOM_SDK_NAMESPACE::AUTHRET_JWTTOKENWRONG:
                std::cout << "JWT token wrong"; break;
            default:
                std::cout << "Other error: " << ret; break;
        }
        std::cout << std::endl;
        
        // Exit the main loop
        if (mainLoop && g_main_loop_is_running(mainLoop)) {
            std::cout << "Quitting GMainLoop..." << std::endl;
            g_main_loop_quit(mainLoop);
        }
    }
    
    void onLoginReturnWithReason(ZOOM_SDK_NAMESPACE::LOGINSTATUS ret, ZOOM_SDK_NAMESPACE::IAccountInfo* pAccountInfo, ZOOM_SDK_NAMESPACE::LoginFailReason reason) override {}
    void onLogout() override {}
    void onZoomIdentityExpired() override {}
    void onZoomAuthIdentityExpired() override {}
};

int main() {
    std::cout << "=== Zoom SDK Authentication Test with GMainLoop ===" << std::endl;
    
    // Initialize GLib
    GMainLoop* mainLoop = g_main_loop_new(nullptr, FALSE);
    if (!mainLoop) {
        std::cerr << "Failed to create GMainLoop" << std::endl;
        return -1;
    }
    std::cout << "✓ Created GMainLoop" << std::endl;

    // Initialize SDK
    const char* sdkPath = "/workspaces/zoom-bot/zoom-sdk";
    setenv("LD_LIBRARY_PATH", sdkPath, 1);

    ZOOM_SDK_NAMESPACE::InitParam initParam;
    initParam.strWebDomain = "https://zoom.us";
    initParam.emLanguageID = ZOOM_SDK_NAMESPACE::LANGUAGE_English;
    initParam.enableLogByDefault = true;
    initParam.enableGenerateDump = true;
    initParam.uiLogFileSize = 10;
    
    if (ZOOM_SDK_NAMESPACE::InitSDK(initParam) != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS) {
        std::cerr << "Failed to initialize Zoom SDK" << std::endl;
        g_main_loop_unref(mainLoop);
        return -1;
    }
    std::cout << "✓ Zoom SDK initialized" << std::endl;

    // Create auth service
    ZOOM_SDK_NAMESPACE::IAuthService* authService = nullptr;
    if (ZOOM_SDK_NAMESPACE::CreateAuthService(&authService) != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS || !authService) {
        std::cerr << "Failed to create auth service" << std::endl;
        g_main_loop_unref(mainLoop);
        return -1;
    }
    std::cout << "✓ Auth service created" << std::endl;

    // Create auth handler with GMainLoop support
    TestAuthHandler authHandler(mainLoop);
    if (authService->SetEvent(&authHandler) != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS) {
        std::cerr << "Failed to set auth event handler" << std::endl;
        g_main_loop_unref(mainLoop);
        return -1;
    }
    std::cout << "✓ Auth event handler registered" << std::endl;

    // Create JWT token for authentication
    std::string sdkKey = "2YAIdaERS82YdStrg6iwuQ";
    std::string sdkSecret = "bi996BXSPNrEaiGJXVh6ckCzdoNeJtKA";
    
    time_t now = time(nullptr);
    time_t exp = now + 3600;

    nlohmann::json payload = {
        {"appKey", sdkKey},
        {"iat", now},
        {"exp", exp},
        {"tokenExp", exp}
    };

    const std::string headerStr = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9";
    std::string payloadStr = base64url_encode(payload.dump());
    std::string signInput = headerStr + "." + payloadStr;
    std::string signature = base64url_encode(hmacSha256(signInput, sdkSecret));
    std::string jwtToken = signInput + "." + signature;

    // Perform authentication
    ZOOM_SDK_NAMESPACE::AuthContext authContext;
    authContext.jwt_token = jwtToken.c_str();
    
    std::cout << "\n🔐 Starting authentication..." << std::endl;
    std::cout << "JWT: " << jwtToken.substr(0, 50) << "..." << std::endl;
    
    auto authResult = authService->SDKAuth(authContext);
    if (authResult != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS) {
        std::cerr << "Failed to initiate SDK authentication: " << authResult << std::endl;
        g_main_loop_unref(mainLoop);
        return -1;
    }
    
    std::cout << "✓ Authentication request sent" << std::endl;

    // Set up timeout
    guint timeoutId = g_timeout_add_seconds(15, [](gpointer data) -> gboolean {
        GMainLoop* loop = static_cast<GMainLoop*>(data);
        std::cout << "\n⏰ Authentication timeout (15s)" << std::endl;
        g_main_loop_quit(loop);
        return FALSE;
    }, mainLoop);

    std::cout << "\n⏳ Running GMainLoop to wait for callback..." << std::endl;
    std::cout << "(This will demonstrate that callbacks work with GMainLoop)\n" << std::endl;
    
    // Run the event loop
    g_main_loop_run(mainLoop);
    
    // Cleanup timeout
    if (timeoutId > 0) {
        g_source_remove(timeoutId);
    }
    
    std::cout << "\n=== Results ===" << std::endl;
    if (authHandler.authenticationCompleted) {
        std::cout << "✅ Authentication callback WAS RECEIVED!" << std::endl;
        std::cout << "Final result: " << authHandler.lastResult << std::endl;
    } else {
        std::cout << "❌ Authentication callback was NOT received (timeout)" << std::endl;
    }

    // Cleanup
    g_main_loop_unref(mainLoop);
    std::cout << "\n✓ Cleanup complete" << std::endl;
    
    return 0;
}