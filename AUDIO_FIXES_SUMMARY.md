# Audio System Fixes for Zoom Bot

## Issues Resolved

### 1. VoIP Join Failed (Error Code 2)
**Problem**: `SDKERR_WRONG_USAGE` when trying to join VoIP immediately after meeting join
**Solution**: 
- Added timing delays to wait for meeting stabilization
- Improved meeting state detection before VoIP join
- Added retry logic for failed VoIP joins

### 2. Virtual Audio Device Detection
**Problem**: Zoom SDK couldn't find audio devices in container environment
**Solution**:
- Created proper ALSA configuration (`/etc/asound.conf`)
- Added ALSA PulseAudio plugin (`libasound2-plugins`) 
- Set environment variables for audio discovery
- Improved PulseAudio startup reliability

### 3. Recording Permission Request
**Problem**: Bot wasn't requesting host to start recording
**Solution**:
- Added explicit recording permission request before audio setup
- Improved error handling and user feedback
- Added timing for recording request processing

### 4. Audio System Integration
**Problem**: ALSA errors and audio device conflicts
**Solution**:
- Created unified launcher script (`run-zoom-bot.sh`)
- Automatic virtual audio setup before bot start
- Better error handling and fallback mechanisms
- Container-optimized audio environment

## New Files Created

1. **`run-zoom-bot.sh`** - Launcher with automatic audio setup
2. **`asound.conf`** - ALSA configuration for PulseAudio integration
3. **`test-audio-system.sh`** - Audio system diagnostics
4. **Updated Dockerfile** - Audio packages and configurations
5. **Updated docker-entrypoint.sh`** - Container audio initialization

## Code Changes

### Audio Manager (`src/audio_manager.cpp`)
- Added `waitForMeetingStable()` function
- Improved VoIP join with better error handling
- Added timing delays and retry logic

### Main Application (`src/main.cpp`) 
- Modified `setupAudioRecording()` to request recording permission
- Added delays for meeting stabilization
- Improved error handling and retry logic

### Dockerfile Enhancements
- Added ALSA PulseAudio plugin
- Audio environment variables
- ALSA configuration setup
- Launcher script integration

## Usage

### Recommended: Use the Launcher
```bash
./run-zoom-bot.sh
```

### Manual Setup (if needed)
```bash
./setup-virtual-audio.sh
cd build && ./zoom_poc
```

### In Docker Container
```bash
docker run zoom-bot zoom_poc
# OR
docker run zoom-bot /usr/local/bin/run-zoom-bot.sh
```

## Expected Improvements

1. **VoIP Join Success**: Error code 2 should be resolved
2. **Recording Requests**: Bot will ask host to start recording
3. **No ALSA Errors**: Clean audio system startup
4. **Reliable Audio**: Virtual devices properly detected by Zoom SDK

## Troubleshooting

If issues persist:
1. Check virtual audio setup: `./test-virtual-audio.sh`
2. Verify audio system: `./test-audio-system.sh`  
3. Manual audio setup: `./setup-virtual-audio.sh`
4. Check PulseAudio: `pactl info`

## Testing

The launcher script provides detailed status output to verify:
- PulseAudio startup
- Virtual device creation
- Audio environment configuration
- Bot startup with audio support