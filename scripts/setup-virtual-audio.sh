#!/bin/bash

# Setup Virtual Audio Devices for Zoom Bot
# This script creates virtual audio devices needed for the Zoom bot to function 
# in containerized environments without physical audio hardware.

set -e

echo "[AUDIO] Setting up virtual audio devices for Zoom bot..."

# Function to wait for PulseAudio to be ready
wait_for_pulseaudio() {
    local max_attempts=30
    local attempt=0
    
    while [ $attempt -lt $max_attempts ]; do
        if pactl info >/dev/null 2>&1; then
            echo "[AUDIO] PulseAudio is ready"
            return 0
        fi
        
        echo "[AUDIO] Waiting for PulseAudio... (attempt $((attempt + 1))/$max_attempts)"
        sleep 1
        attempt=$((attempt + 1))
    done
    
    echo "[AUDIO] ERROR: PulseAudio failed to start within $max_attempts seconds"
    return 1
}

# Function to setup virtual devices
setup_virtual_devices() {
    echo "[AUDIO] Creating virtual speaker (output device)..."
    pactl load-module module-null-sink \
        sink_name=virtual_speaker \
        sink_properties=device.description=Virtual_Speaker \
        rate=44100 \
        channels=2 \
        channel_map=stereo

    echo "[AUDIO] Creating virtual microphone (input device)..."
    pactl load-module module-virtual-source \
        source_name=virtual_microphone \
        rate=44100 \
        channels=2 \
        channel_map=stereo

    echo "[AUDIO] Setting virtual devices as defaults..."
    pactl set-default-sink virtual_speaker
    pactl set-default-source virtual_microphone

    echo "[AUDIO] Virtual audio devices configured successfully!"
    
    # Display configuration
    echo "[AUDIO] Current audio configuration:"
    echo "  Default Sink: $(pactl info | grep 'Default Sink:' | cut -d' ' -f3)"
    echo "  Default Source: $(pactl info | grep 'Default Source:' | cut -d' ' -f3)"
    
    # List available devices
    echo "[AUDIO] Available sinks:"
    pactl list short sinks | sed 's/^/  /'
    echo "[AUDIO] Available sources:"
    pactl list short sources | sed 's/^/  /'
}

# Start PulseAudio if not running
if ! pgrep -x "pulseaudio" > /dev/null; then
    echo "[AUDIO] Starting PulseAudio daemon..."
    
    # Kill any existing processes first
    killall pulseaudio 2>/dev/null || true
    
    # Start PulseAudio daemon
    if [ "$(id -u)" = "0" ]; then
        # Running as root - use user mode with proper environment
        export PULSE_RUNTIME_PATH=/tmp/pulse-runtime
        mkdir -p $PULSE_RUNTIME_PATH
        pulseaudio --start --log-level=info --verbose &
        sleep 2
    else
        # Running as regular user
        pulseaudio --start --log-level=info
    fi
fi

# Wait for PulseAudio to be ready
wait_for_pulseaudio

# Setup virtual devices
setup_virtual_devices

echo "[AUDIO] Virtual audio setup complete!"