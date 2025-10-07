# Clean Shutdown Feature Implementation

## Overview
We have successfully implemented a clean shutdown feature that allows the Zoom bot to gracefully stop recording and exit the meeting when receiving termination signals (Ctrl+C or SIGTERM).

## Implementation Details

### Signal Handling
- **Signal Registration**: SIGINT (Ctrl+C) and SIGTERM signals are registered in `main()` function
- **Global State**: Thread-safe atomic boolean `shouldExit` tracks shutdown request
- **Global Pointers**: `globalAudioHandler` and `globalMeetingService` provide access for cleanup

### Shutdown Sequence
1. **Signal Reception**: User presses Ctrl+C or system sends SIGTERM
2. **Signal Handler Execution**: `signalHandler()` function runs:
   - Sets `shouldExit` flag to true
   - Calls `stopRecording()` to stop raw recording via `StopRawRecording()`
   - Attempts to leave meeting via `LeaveMeeting()`
   - Provides detailed logging of shutdown steps

### Main Loop Integration
- **Exit Condition**: Main loop now checks both `meetingJoined` and `!shouldExit.load()`
- **Graceful Detection**: Shutdown message displays when exiting due to signal
- **Pointer Cleanup**: Global pointers are cleared after use

## Usage
1. Start the bot: `./zoom_poc`
2. Bot joins meeting and starts recording (with host permission)
3. Press **Ctrl+C** to initiate clean shutdown
4. Bot will:
   - Stop raw recording
   - Unsubscribe from audio streams
   - Leave the meeting
   - Clean up resources
   - Exit gracefully

## Code Changes Made
- Added signal handling includes (`<csignal>`, `<atomic>`)
- Implemented global variables for shutdown coordination
- Created `signalHandler()` function with comprehensive cleanup
- Updated main loop to respect `shouldExit` flag
- Added pointer management for safe cleanup

## Benefits
- **No Data Loss**: Recording stops cleanly without corruption
- **Meeting Courtesy**: Bot leaves meeting properly, not just disconnecting
- **Resource Cleanup**: All SDK resources are properly released
- **User Control**: Immediate response to Ctrl+C for quick exit
- **Debugging**: Comprehensive logging of shutdown process

## Testing
The implementation has been compiled successfully and is ready for testing in a live meeting environment.