#pragma once

#include <iostream>
#include <glib.h>
#include "zoom_sdk.h"
#include "zoom_sdk_def.h"
#include "auth_service_interface.h"

namespace ZoomBot {

class AuthEventHandler : public ZOOM_SDK_NAMESPACE::IAuthServiceEvent {
public:
    bool authenticationCompleted = false;
    ZOOM_SDK_NAMESPACE::AuthResult lastResult = ZOOM_SDK_NAMESPACE::AUTHRET_NONE;
    GMainLoop* mainLoop = nullptr;

    explicit AuthEventHandler(GMainLoop* loop);

    // IAuthServiceEvent overrides
    void onAuthenticationReturn(ZOOM_SDK_NAMESPACE::AuthResult ret) override;
    void onLoginReturnWithReason(ZOOM_SDK_NAMESPACE::LOGINSTATUS ret, ZOOM_SDK_NAMESPACE::IAccountInfo* pAccountInfo, ZOOM_SDK_NAMESPACE::LoginFailReason reason) override;
    void onLogout() override;
    void onZoomIdentityExpired() override;
    void onZoomAuthIdentityExpired() override;

private:
    void logAuthResult(ZOOM_SDK_NAMESPACE::AuthResult ret);
};

} // namespace ZoomBot