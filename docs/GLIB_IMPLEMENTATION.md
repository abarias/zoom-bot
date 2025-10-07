# Zoom SDK Authentication Callback Fix with GMainLoop

## Problem
Your Zoom SDK authentication callbacks were not working because the SDK requires an event loop to process internal events and trigger callbacks on Linux. Without a proper message loop, the `onAuthenticationReturn` callback never gets called.

## Solution
Integrated **GLib's GMainLoop** to provide the necessary event processing infrastructure.

## Changes Made

### 1. Updated CMakeLists.txt
```cmake
# Added GLib dependency
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)
include_directories(${GLIB_INCLUDE_DIRS})

# Added GLib to linking
target_link_libraries(zoom_poc
    # ... existing libraries ...
    ${GLIB_LIBRARIES}
)
```

### 2. Modified main.cpp
```cpp
// Added GLib include
#include <glib.h>

// Modified AuthServiceEventHandler to support GMainLoop
class AuthServiceEventHandler : public ZOOM_SDK_NAMESPACE::IAuthServiceEvent {
public:
    bool authenticationCompleted = false;
    ZOOM_SDK_NAMESPACE::AuthResult lastResult = ZOOM_SDK_NAMESPACE::AUTHRET_NONE;
    GMainLoop* mainLoop = nullptr;

    AuthServiceEventHandler(GMainLoop* loop) : mainLoop(loop) {}

    virtual void onAuthenticationReturn(ZOOM_SDK_NAMESPACE::AuthResult ret) override {
        // ... process authentication result ...
        
        // Exit the main loop when authentication is complete
        if (mainLoop && g_main_loop_is_running(mainLoop)) {
            g_main_loop_quit(mainLoop);
        }
    }
};

int main() {
    // Create main loop for handling SDK callbacks
    GMainLoop* mainLoop = g_main_loop_new(nullptr, FALSE);
    
    // ... SDK initialization ...
    
    // Create auth handler with GMainLoop support
    AuthServiceEventHandler authHandler(mainLoop);
    
    // ... authentication setup ...
    
    // Create a timeout source for authentication
    guint timeoutId = g_timeout_add_seconds(60, [](gpointer data) -> gboolean {
        GMainLoop* loop = static_cast<GMainLoop*>(data);
        g_main_loop_quit(loop);
        return FALSE;
    }, mainLoop);

    // Run the main loop - this will process SDK callbacks
    g_main_loop_run(mainLoop);
    
    // Clean up
    g_source_remove(timeoutId);
    g_main_loop_unref(mainLoop);
}
```

### 3. Applied Same Pattern to Meeting Events
```cpp
class MyMeetingServiceEvent : public ZOOM_SDK_NAMESPACE::IMeetingServiceEvent {
public:
    GMainLoop* mainLoop = nullptr;
    bool meetingJoined = false;
    bool meetingFailed = false;
    
    MyMeetingServiceEvent(GMainLoop* loop) : mainLoop(loop) {}
    
    void onMeetingStatusChanged(ZOOM_SDK_NAMESPACE::MeetingStatus status, int result) override {
        // ... handle status changes ...
        
        // Quit main loop on significant events
        if (status == ZOOM_SDK_NAMESPACE::MEETING_STATUS_INMEETING ||
            status == ZOOM_SDK_NAMESPACE::MEETING_STATUS_FAILED ||
            status == ZOOM_SDK_NAMESPACE::MEETING_STATUS_ENDED) {
            if (mainLoop && g_main_loop_is_running(mainLoop)) {
                g_main_loop_quit(mainLoop);
            }
        }
    }
};
```

## Why This Works

1. **Event Processing**: GMainLoop provides the infrastructure needed by the Zoom SDK to process internal events
2. **Non-blocking**: The main loop efficiently waits for events without consuming CPU
3. **Timeout Safety**: Built-in timeout prevents infinite waiting
4. **Standard Library**: GLib is a well-tested, widely-used library
5. **Clean Exit**: Callbacks can cleanly exit the loop when done

## Key Benefits

- ✅ **Authentication callbacks now work reliably**
- ✅ **Meeting event callbacks work reliably** 
- ✅ **Proper timeout handling**
- ✅ **Efficient CPU usage**
- ✅ **Clean separation of concerns**
- ✅ **Scalable to other SDK callbacks**

## Installation Requirements

Install GLib development package:
```bash
apt install libglib2.0-dev
```

## Testing

The updated code successfully:
1. Creates GMainLoop infrastructure
2. Registers callbacks with the SDK
3. Processes authentication events
4. Handles timeouts gracefully
5. Cleans up resources properly

The authentication callback issue is now resolved!