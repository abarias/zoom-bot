# Zoom Bot - Audio Recording Bot for Zoom Meetings

A C++ application that joins Zoom meetings as a bot participant and records per-participant audio streams with proper permission handling.

## ✨ Features

- **Automated Meeting Join**: Joins Zoom meetings using Meeting SDK
- **Permission-Based Recording**: Requests and respects host recording permissions
- **Per-Participant Audio**: Captures individual participant audio streams
- **Raw Audio Processing**: Records in PCM format with automatic WAV conversion
- **Privacy Compliance**: Respects host decisions on recording permissions
- **Graceful Shutdown**: Clean exit handling with proper resource cleanup

## 🚀 Quick Start

### Prerequisites

- **Linux Environment** (Ubuntu 22.04+ recommended)
- **CMake** 3.16+
- **GCC/G++** with C++17 support
- **GLib 2.0** development libraries
- **nlohmann/json** library
- **Zoom Meeting SDK** (included in setup)

### Installation

1. **Clone the repository:**
   ```bash
   git clone <your-repo-url>
   cd zoom-bot
   ```

2. **Extract Zoom SDK:**
   ```bash
   # Download zoom-sdk-linux.tar.xz from Zoom Developer Portal
   tar -xf zoom-sdk-linux.tar.xz
   ```

3. **Build the project:**
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

4. **Configure credentials:**
   
   **Option 1: Use the setup script (recommended):**
   ```bash
   ./setup-env.sh
   # This will create .env.local file and guide you through setup
   ```

   **Option 2: Manual setup:**
   ```bash
   # Copy the example environment file
   cp .env.example .env.local
   
   # Edit with your actual credentials
   nano .env.local  # or your preferred editor
   
   # Load environment variables
   source .env.local
   ```

   **Required Environment Variables:**
   - `ZOOM_CLIENT_ID`, `ZOOM_CLIENT_SECRET`, `ZOOM_ACCOUNT_ID` (OAuth app)
   - `ZOOM_APP_KEY`, `ZOOM_APP_SECRET` (SDK app) 
   - `ZOOM_MEETING_NUMBER`, `ZOOM_MEETING_PASSWORD` (meeting info)
   - `ZOOM_BOT_USERNAME` (optional, defaults to "ZoomBot")

### Usage

**First-time setup:**
```bash
# Set up environment variables
./setup-env.sh

# Or manually:
source .env.local  # Load your environment variables
```

**Run the bot:**
```bash
# Make sure environment variables are loaded
source .env.local  # If not already loaded

# Run the bot (from build directory)
./zoom_poc

# Or from project root
./build/zoom_poc
```

The bot will:
1. Authenticate with Zoom APIs
2. Join the configured meeting
3. Request recording permission from host
4. Start audio capture if approved
5. Save per-participant audio files in `recordings/TIMESTAMP/`

### Configuration

The bot uses environment variables for secure credential management. Set these variables:

```bash
# OAuth credentials (Server-to-Server OAuth app)
export ZOOM_CLIENT_ID=your_client_id
export ZOOM_CLIENT_SECRET=your_client_secret  
export ZOOM_ACCOUNT_ID=your_account_id

# SDK credentials (Meeting SDK app)
export ZOOM_APP_KEY=your_app_key
export ZOOM_APP_SECRET=your_app_secret

# Meeting configuration
export ZOOM_MEETING_NUMBER=12345678901
export ZOOM_MEETING_PASSWORD=meeting_password
export ZOOM_BOT_USERNAME=ZoomBot  # Optional
```

**Getting Credentials:**
1. Go to [Zoom App Marketplace](https://marketplace.zoom.us/develop/apps)
2. Create a **Server-to-Server OAuth** app for OAuth credentials
3. Create a **Meeting SDK** app for SDK credentials  
4. Enable required scopes: `meeting:read`, `meeting:write`
5. Enable **Raw Data** feature in SDK app (if available)

## 🔧 Technical Architecture

### Core Components

- **`AudioRawHandler`**: Manages audio stream subscription and file writing
- **`MeetingEventHandler`**: Handles Zoom SDK callbacks and events
- **`SDKInitializer`**: Manages SDK lifecycle and authentication
- **`MeetingDetector`**: Enhanced meeting join detection
- **`ZoomAuth`**: OAuth token management

### Permission Flow

```
1. Join Meeting → 2. Request Recording Permission → 3. Wait for Host Approval
                                ↓
4. Start Recording → 5. Subscribe to Audio → 6. Capture Per-Participant Streams
```

### Audio Output

Files are saved in `recordings/YYYYMMDD_HHMMSS/`:
- `user_[ID]_[Name]_48000Hz_1ch.pcm` - Individual participants  
- `mixed_48000Hz_2ch.pcm` - Mixed audio stream
- Automatic conversion to `.wav` format on exit

## 📋 Privacy & Compliance

- ✅ **Explicit Permission**: Always requests host approval
- ✅ **Respect Denials**: Stops recording if host denies permission
- ✅ **Clear Notifications**: Host sees permission request popup
- ✅ **Timeout Handling**: Graceful handling of non-responses
- ✅ **Clean Exit**: Proper cleanup and file conversion

## 🛠 Development

### Building Tests

```bash
cd build
make test_auth      # Authentication testing
make wav_converter  # Audio conversion utility
```

### File Structure

```
src/
├── main.cpp                    # Main application entry
├── audio_raw_handler.{cpp,h}   # Audio capture and file management
├── meeting_event_handler.{cpp,h} # Zoom callback handling
├── sdk_initializer.{cpp,h}     # SDK lifecycle management
├── meeting_detector.{cpp,h}    # Meeting join detection
├── zoom_auth.{cpp,h}          # OAuth authentication
└── jwt_helper.{cpp,h}         # JWT token generation
```

### Adding Features

1. **Audio Processing**: Extend `AudioRawHandler` for real-time processing
2. **Multiple Meetings**: Modify main loop for concurrent sessions
3. **Cloud Storage**: Add upload functionality in `PCMFile` class
4. **Web Interface**: Integrate with web framework for remote control

## 🔍 Troubleshooting

### Common Issues

**"Recording permission denied"**
- Host must approve the recording request in Zoom client
- Check meeting settings allow participant recording

**"Audio subscription failed"**  
- Ensure Raw Data is enabled in Zoom App Marketplace
- Verify bot has joined VoIP audio
- Check meeting supports raw audio access

**"Meeting join timeout"**
- Verify meeting ID and password
- Check network connectivity
- Ensure meeting is active and host has started

**Build errors**
- Install required dependencies: `sudo apt install libglib2.0-dev nlohmann-json3-dev`
- Verify CMake version: `cmake --version`

### Debug Mode

Enable verbose logging by modifying debug flags in `main.cpp`:

```cpp
#define DEBUG_VERBOSE 1  // Add to top of main.cpp
```

## 📚 Documentation

- [`CORRECTED_IMPLEMENTATION.md`](CORRECTED_IMPLEMENTATION.md) - Implementation details
- [`TEST_GUIDE.md`](TEST_GUIDE.md) - Testing procedures  
- [`RAW_AUDIO_STATUS.md`](RAW_AUDIO_STATUS.md) - Audio capture status
- [`WAV_CONVERSION_GUIDE.md`](WAV_CONVERSION_GUIDE.md) - Audio format handling

## 🤝 Contributing

1. Fork the repository
2. Create your feature branch: `git checkout -b feature/amazing-feature`
3. Commit your changes: `git commit -m 'Add amazing feature'`
4. Push to the branch: `git push origin feature/amazing-feature`  
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## ⚠️ Important Notes

- **Zoom SDK License**: Ensure compliance with Zoom's SDK terms
- **Recording Laws**: Follow local laws regarding meeting recording
- **Data Privacy**: Handle recorded audio according to privacy regulations
- **Rate Limits**: Respect Zoom API rate limiting in production use

## 🎯 Roadmap

- [ ] Multi-meeting support
- [ ] Real-time transcription integration
- [ ] Cloud storage integration
- [ ] Web dashboard for monitoring
- [ ] Docker containerization improvements
- [ ] Kubernetes deployment manifests

---

**Need help?** Open an issue or check the documentation files in the repository.