#pragma once

#include <iostream>
#include <string>
#include <cstdlib>
#include <glib.h>
#include "zoom_sdk.h"
#include "zoom_sdk_def.h"
#include "auth_service_interface.h"
#include "meeting_service_interface.h"
#include "auth_event_handler.h"

namespace ZoomBot {

class SDKInitializer {
public:
    struct InitResult {
        bool success = false;
        ZOOM_SDK_NAMESPACE::IAuthService* authService = nullptr;
        ZOOM_SDK_NAMESPACE::IMeetingService* meetingService = nullptr;
        std::string errorMessage;
    };

    static InitResult initializeSDK();
    static bool authenticateSDK(
        ZOOM_SDK_NAMESPACE::IAuthService* authService,
        AuthEventHandler* authHandler,
        GMainLoop* mainLoop,
        const std::string& jwtToken
    );
    static void cleanup(const InitResult& result);

private:
    static void setupEnvironment();
    static void preloadLibraries();
    static ZOOM_SDK_NAMESPACE::InitParam createInitParams();
};

} // namespace ZoomBot