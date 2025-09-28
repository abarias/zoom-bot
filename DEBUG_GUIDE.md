# C++ Debugging Guide for Zoom Bot

## ✅ Debug Setup Complete!

Your workspace is now configured for C++ debugging with the following features:

### **What's Been Set Up:**
1. **Debug Build Configuration**: CMake configured with `-g -O0 -DDEBUG` flags
2. **Launch Configurations**: 3 debugging profiles in `.vscode/launch.json`
3. **Build Tasks**: Automated debug/release build tasks in `.vscode/tasks.json`
4. **IntelliSense**: Enhanced C++ language support with proper include paths
5. **Debug Symbols**: Binary built with full debugging symbols (4.2MB with debug info)

## 🚀 How to Debug:

### **Method 1: VS Code Debug Panel**
1. Open the **Debug panel** (Ctrl+Shift+D)
2. Select **"Debug Zoom Bot"** from the dropdown
3. Click the **Play button** or press **F5**
4. The debugger will launch with environment variables set

### **Method 2: Keyboard Shortcuts**
- **F5**: Start debugging
- **F9**: Toggle breakpoint
- **F10**: Step over
- **F11**: Step into
- **Shift+F11**: Step out
- **Ctrl+Shift+F5**: Restart debugging

### **Method 3: Build and Debug Tasks**
- **Ctrl+Shift+P** → "Tasks: Run Task" → "Build Debug"
- **Ctrl+Shift+P** → "Tasks: Run Task" → "Clean Build"

## 🔧 Available Debug Configurations:

### **1. Debug Zoom Bot** (Primary)
- Launches `zoom_poc` with full debugging
- Sets proper `LD_LIBRARY_PATH` for Zoom SDK
- Working directory: `build/`
- Console input enabled for meeting details

### **2. Debug Test Auth** 
- Launches `test_auth` utility for credential testing
- Useful for debugging OAuth/JWT issues
- Lighter weight than full bot

### **3. Attach to Running Process**
- Attach to an already running `zoom_poc` process
- Useful when debugging live sessions

## 📍 Setting Breakpoints:

### **Key Places to Debug:**
```cpp
// Authentication flow
src/main.cpp:483 - JWT token generation
src/zoom_auth.cpp:45 - OAuth token request

// Meeting joining
src/main.cpp:390 - Console input processing
src/main.cpp:520 - Meeting join attempt
src/meeting_event_handler.cpp:8 - Meeting status callbacks

// Audio processing  
src/audio_raw_handler.cpp:305 - Audio data reception
src/audio_streamer.cpp:95 - TCP streaming
src/main.cpp:651 - Audio subscription success
```

### **Interactive Debugging Tips:**
1. **Set breakpoints** in console input functions to debug parsing:
   - `getMeetingDetailsFromConsole()` - line ~340
   - `parseMeetingNumber()` - line ~315

2. **Debug streaming issues** by breaking in:
   - `AudioRawHandler::enableStreaming()` 
   - `TCPStreamingBackend::connectToServer()`

3. **Monitor meeting state** with breakpoints in:
   - `MeetingEventHandler::onMeetingStatusChanged()`
   - `MeetingDetector::checkMeetingStatus()`

## 🔍 Debug Variables to Watch:

Add these to your **Watch panel**:
```cpp
shouldExit.load()           // Global exit flag
Config::getMeetingNumber()  // Parsed meeting number
globalAudioHandler         // Audio handler pointer
meetingEventHandler.meetingJoined  // Join status
```

## ⚡ Quick Debug Commands:

### **Build for Debugging:**
```bash
# From workspace root
rm -rf build && mkdir build
cd build && cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

### **Debug with GDB directly:**
```bash
cd build
gdb ./zoom_poc
(gdb) break main
(gdb) run
```

### **Check Debug Symbols:**
```bash
objdump -h build/zoom_poc | grep debug  # Should show debug sections
```

## 🐛 Common Debug Scenarios:

### **Meeting Join Issues:**
1. Set breakpoint at `joinMeeting()` (line ~162)
2. Check `normalUserParam` values
3. Step through SDK authentication

### **Audio Streaming Problems:**
1. Break in `AudioRawHandler::enableStreaming()`
2. Watch TCP connection status in `connectToServer()`
3. Monitor audio queue in `AudioStreamer::queueAudio()`

### **Console Input Debugging:**
1. Break in `getMeetingDetailsFromConsole()`
2. Watch `meetingInput` and `parseMeetingNumber()` result
3. Check format validation logic

## 📊 Debug Output:

The application uses structured logging with prefixes:
- `[DEBUG]` - Debug information
- `[TCP]` - Network operations  
- `[STREAMING]` - Audio streaming
- `[AUDIO]` - Audio processing
- `[SHUTDOWN]` - Cleanup operations

Happy debugging! 🎯