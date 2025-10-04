# Virtual Audio Setup for Zoom Bot

This document explains the virtual audio configuration for running the Zoom Bot in containerized environments without physical audio hardware.

## Overview

The Zoom Bot requires audio devices to join meetings with audio capabilities. In containerized environments (like dev containers or Docker), physical audio hardware is not available. To solve this, we create virtual audio devices using PulseAudio.

## Virtual Audio Devices

The setup creates two virtual audio devices:

1. **Virtual Speaker** (`virtual_speaker`)
   - Type: Null sink (audio output device)
   - Purpose: Receives audio from Zoom meetings
   - Specifications: 16-bit stereo, 44.1kHz sample rate

2. **Virtual Microphone** (`virtual_microphone`)
   - Type: Virtual source (audio input device)  
   - Purpose: Provides audio input to Zoom meetings
   - Specifications: 32-bit float stereo, 44.1kHz sample rate

## Files

- `setup-virtual-audio.sh` - Manual setup script for virtual audio devices
- `test-virtual-audio.sh` - Test script to verify virtual audio configuration
- `pulseaudio-virtual.pa` - PulseAudio configuration file for virtual devices
- `docker-entrypoint.sh` - Container entrypoint that automatically sets up virtual audio

## Automatic Setup (Docker)

When using the Docker container, virtual audio devices are automatically configured:

1. Container starts with `docker-entrypoint.sh`
2. PulseAudio daemon is started
3. Virtual audio devices are created
4. Devices are set as system defaults
5. Main application is launched

## Manual Setup

If you need to manually configure virtual audio:

```bash
# Run the setup script
./setup-virtual-audio.sh

# Or manually create devices
pactl load-module module-null-sink sink_name=virtual_speaker
pactl load-module module-virtual-source source_name=virtual_microphone
pactl set-default-sink virtual_speaker
pactl set-default-source virtual_microphone
```

## Verification

Test the virtual audio setup:

```bash
# Run the test script
./test-virtual-audio.sh

# Or manually check
pactl info | grep -E "(Default Sink|Default Source)"
pactl list short sinks
pactl list short sources
```

## Integration with Zoom SDK

The Zoom SDK will automatically detect and use these virtual devices:

- **Audio Output**: Meeting audio will be sent to `virtual_speaker`
- **Audio Input**: The bot's audio will be read from `virtual_microphone`
- **Recording**: Audio can be captured from both devices for processing

## Audio Processing Pipeline

```
Zoom Meeting Audio → virtual_speaker → Audio Processing → TCP Stream/Files
Bot Audio Input ← virtual_microphone ← Audio Generation/Playback
```

## Troubleshooting

### PulseAudio Not Starting
- Check if running as root (use `--system` flag or run as user)
- Verify PulseAudio packages are installed
- Check for permission issues with `/tmp/pulse-*` directories

### Virtual Devices Missing
- Run `./setup-virtual-audio.sh` to recreate devices
- Check PulseAudio modules: `pactl list modules | grep -E "(null-sink|virtual-source)"`
- Restart PulseAudio: `pulseaudio --kill && pulseaudio --start`

### Audio Not Working in Zoom Bot
- Verify devices are set as defaults: `pactl info`
- Check Zoom SDK audio initialization in application logs
- Test audio flow: `pactl list sink-inputs` and `pactl list source-outputs`

## Development Notes

- Virtual devices persist only while PulseAudio is running
- Container restart will recreate devices automatically
- Audio data is processed in memory (no actual sound hardware needed)
- Sample rates and formats can be adjusted in the setup scripts if needed

## Running the Bot

### Using the Launcher Script (Recommended)
```bash
# The launcher automatically sets up virtual audio and starts the bot
./run-zoom-bot.sh

# In Docker container
docker run zoom-bot zoom_poc
```

### Manual Setup
```bash
# Set up virtual audio first
./setup-virtual-audio.sh

# Then run the bot normally  
cd build && ./zoom_poc
```

## Commands Reference

```bash
# List all sinks (output devices)
pactl list short sinks

# List all sources (input devices)  
pactl list short sources

# Show current defaults
pactl info | grep -E "(Default Sink|Default Source)"

# Change defaults
pactl set-default-sink <sink_name>
pactl set-default-source <source_name>

# Monitor audio activity
pactl list sink-inputs    # Active audio outputs
pactl list source-outputs # Active audio inputs

# Test audio system
./test-audio-system.sh
./test-virtual-audio.sh
```