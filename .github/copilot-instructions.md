# Zoom Bot - AI Coding Agent Instructions

## Architecture Overview

This is a **C++ Zoom Meeting Bot** with **real-time audio streaming** capabilities. The system uses a modular architecture:

```
Zoom SDK → AudioRawHandler → [File Storage] + [TCP Stream] → Python Service → WAV Files
```

### Key Components
- **Core C++ Bot** (`src/main.cpp`): Meeting lifecycle, authentication, signal handling
- **Audio Pipeline** (`src/audio_raw_handler.*`): Dual-output audio capture (files + streaming)
- **TCP Streaming** (`src/audio_streamer.*`): Pluggable backend architecture with worker threads
- **Python Service** (`audio_processor.py`): TCP server for real-time audio processing
- **Configuration** (`src/config.*`): Environment + runtime meeting details

## Critical Patterns

### 1. **Zoom SDK Integration Pattern**
- All SDK interactions use `ZOOM_SDK_NAMESPACE::` prefix
- SDK lifecycle: `Initialize → Authenticate → Join → Subscribe → Leave → Cleanup`
- Essential includes: `meeting_service_interface.h`, `meeting_audio_interface.h`

### 2. **Audio Data Flow**
```cpp
// AudioRawHandler receives SDK callbacks and routes data
void onOneAudioDataReceived(AudioRawData* data_) {
    writeToFile(userFiles_[user_id], data_);      // Local PCM file
    streamAudioData(user_id, user_name, data_);   // TCP streaming
}
```

### 3. **Console Input Pattern**
Meeting credentials are collected interactively, not from environment:
```cpp
// Format: "123 4567 8901" → cleaned to "12345678901"
std::string parseMeetingNumber(const std::string& input);
bool getMeetingDetailsFromConsole(std::string& meetingNumber, std::string& password);
```

### 4. **Logging Convention**
Use structured prefixes for different subsystems:
```cpp
std::cout << "[TCP] Connected to server" << std::endl;        // Network operations
std::cout << "[STREAMING] Queue size: " << size << std::endl; // Audio streaming
std::cout << "[AUDIO] Subscription failed" << std::endl;      // Audio capture
std::cout << "[DEBUG] Exit signal received" << std::endl;     // Debug information
```

### 5. **Error Handling Strategy**
- **Graceful degradation**: Continue with file recording if streaming fails
- **Signal safety**: Use `std::atomic<bool> shouldExit` for clean shutdowns
- **Resource cleanup**: Always call `unsubscribe()` and `shutdown()` in destructors

## Development Workflows

### Build Commands
```bash
mkdir build && cd build
cmake .. && make
# Specific targets: make test_auth, make wav_converter
```

### Testing TCP Integration
```bash
./scripts/start_audio_service.sh            # Start Python service
python3 tests/test_connection.py            # Test TCP connectivity
cd build && ./zoom_poc                      # Run main bot
```

### Environment Setup
```bash
./setup-env.sh                             # Interactive credential setup
source .env.local                          # Load environment variables
# Runtime meeting details are collected via console, not env vars
```

## Integration Points

### Streaming Backend Interface
```cpp
class StreamingBackend {
    virtual bool initialize(const std::string& config) = 0;
    virtual bool streamAudio(uint32_t user_id, const std::string& user_name, 
                           const char* data, size_t length, 
                           uint32_t sample_rate, uint16_t channels) = 0;
};
```

### TCP Protocol (TLV Format)
```
[Header Size: 4 bytes] → [Header JSON: variable] → [Data Size: 4 bytes] → [PCM Audio: variable]
```

### Python Service Configuration
```bash
python3 audio_processor.py --host localhost --port 8888 --output-dir processed_audio --verbose
```

## Project-Specific Conventions

### File Structure
- **Core logic**: `src/` - Single responsibility classes with `.h/.cpp` pairs
- **Audio output**: `recordings/YYYYMMDD_HHMMSS/` (local PCM files)
- **Streaming output**: `processed_audio/` (Python service WAV files)
- **Scripts**: Root directory - `.sh` files for common workflows

### Configuration Approach
- **Credentials**: Environment variables (`ZOOM_CLIENT_ID`, `ZOOM_APP_KEY`, etc.)
- **Meeting details**: Interactive console input with format validation
- **Runtime settings**: Config class with both static loading and runtime setters

### Threading Model
- **Main thread**: Zoom SDK callbacks and GMainLoop processing
- **Worker thread**: Audio streaming queue processing (`AudioStreamer::workerLoop`)
- **Signal handling**: Global atomic flags for graceful shutdown across threads

### Hostname Resolution
Always use `gethostbyname()` fallback when `inet_pton()` fails:
```cpp
if (inet_pton(AF_INET, host.c_str(), &addr) <= 0) {
    struct hostent* host_entry = gethostbyname(host.c_str());
    // Handle hostname resolution...
}
```

This pattern is essential for "localhost" and other hostname-based connections.