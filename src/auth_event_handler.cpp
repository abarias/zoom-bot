#include "auth_event_handler.h"

namespace ZoomBot {

AuthEventHandler::AuthEventHandler(GMainLoop* loop) : mainLoop(loop) {}

void AuthEventHandler::onAuthenticationReturn(ZOOM_SDK_NAMESPACE::AuthResult ret) {
    std::cout << "\n[AUTH CALLBACK] Received authentication result: " << ret << std::endl;
    
    authenticationCompleted = true;
    lastResult = ret;
    
    std::cout << "[AUTH CALLBACK] Status: ";
    logAuthResult(ret);
    std::cout << std::endl;
    std::cout.flush();  // Make sure output is displayed immediately
    
    // Exit the main loop when authentication is complete
    if (mainLoop && g_main_loop_is_running(mainLoop)) {
        g_main_loop_quit(mainLoop);
    }
}

void AuthEventHandler::logAuthResult(ZOOM_SDK_NAMESPACE::AuthResult ret) {
    switch(ret) {
        case ZOOM_SDK_NAMESPACE::AUTHRET_SUCCESS:
            std::cout << "Authentication successful"; 
            break;
        case ZOOM_SDK_NAMESPACE::AUTHRET_KEYORSECRETEMPTY:
            std::cout << "Key or secret is empty"; 
            break;
        case ZOOM_SDK_NAMESPACE::AUTHRET_KEYORSECRETWRONG:
            std::cout << "Key or secret is wrong"; 
            break;
        case ZOOM_SDK_NAMESPACE::AUTHRET_ACCOUNTNOTSUPPORT:
            std::cout << "Account does not support"; 
            break;
        case ZOOM_SDK_NAMESPACE::AUTHRET_ACCOUNTNOTENABLESDK:
            std::cout << "Account not enabled for SDK"; 
            break;
        case ZOOM_SDK_NAMESPACE::AUTHRET_UNKNOWN:
            std::cout << "Unknown error"; 
            break;
        case ZOOM_SDK_NAMESPACE::AUTHRET_SERVICE_BUSY:
            std::cout << "Service busy"; 
            break;
        case ZOOM_SDK_NAMESPACE::AUTHRET_NONE:
            std::cout << "Initial status"; 
            break;
        case ZOOM_SDK_NAMESPACE::AUTHRET_OVERTIME:
            std::cout << "Timeout"; 
            break;
        case ZOOM_SDK_NAMESPACE::AUTHRET_NETWORKISSUE:
            std::cout << "Network issues"; 
            break;
        case ZOOM_SDK_NAMESPACE::AUTHRET_CLIENT_INCOMPATIBLE:
            std::cout << "Client incompatible"; 
            break;
        case ZOOM_SDK_NAMESPACE::AUTHRET_JWTTOKENWRONG:
            std::cout << "JWT token wrong"; 
            break;
        default:
            std::cout << "Unrecognized error"; 
            break;
    }
}

// Empty implementations for required virtual functions
void AuthEventHandler::onLoginReturnWithReason(ZOOM_SDK_NAMESPACE::LOGINSTATUS ret, ZOOM_SDK_NAMESPACE::IAccountInfo* pAccountInfo, ZOOM_SDK_NAMESPACE::LoginFailReason reason) {}
void AuthEventHandler::onLogout() {}
void AuthEventHandler::onZoomIdentityExpired() {}
void AuthEventHandler::onZoomAuthIdentityExpired() {}

} // namespace ZoomBot