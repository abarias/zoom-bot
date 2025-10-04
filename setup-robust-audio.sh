#!/bin/bash

# Robust Audio Setup for Zoom Bot in Container
# Fixes ALSA errors and ensures stable PulseAudio operation

echo "[AUDIO] Setting up robust audio environment for container..."

# Kill any existing audio processes
pkill -f pulseaudio 2>/dev/null || true
sleep 2

# Clear any stale runtime files
rm -rf /tmp/pulse-runtime /tmp/pulse-* ~/.pulse* 2>/dev/null || true

# Set environment variables for container audio
export PULSE_RUNTIME_PATH=/tmp/pulse-runtime
export ALSA_PCM_CARD=pulse
export ALSA_PCM_DEVICE=0
export PULSE_PROP="application.process.binary=zoom_poc"

# Ensure proper permissions
mkdir -p /tmp/pulse-runtime
chmod 755 /tmp/pulse-runtime

echo "[AUDIO] Starting PulseAudio with container-optimized settings..."

# Start PulseAudio with specific configuration for containers
pulseaudio --start --log-level=info 2>/dev/null || true
sleep 3

# Verify PulseAudio is running
if ! pactl info >/dev/null 2>&1; then
    echo "[AUDIO] ⚠ PulseAudio not responding, trying alternative startup..."
    
    # Alternative startup method for containers
    pulseaudio --kill 2>/dev/null || true
    sleep 1
    
    # Simple startup without problematic flags
    pulseaudio --start 2>/dev/null || true
    sleep 2
    
    # Last resort - try system mode
    if ! pactl info >/dev/null 2>&1; then
        pulseaudio --system --disallow-exit --disallow-module-loading=false --daemonize 2>/dev/null || true
        sleep 2
    fi
fi

# Create virtual devices with error suppression
echo "[AUDIO] Creating virtual devices..."

pactl load-module module-null-sink \
    sink_name=virtual_speaker \
    sink_properties=device.description=Virtual_Speaker \
    2>/dev/null || echo "[AUDIO] Virtual speaker already exists"

pactl load-module module-virtual-source \
    source_name=virtual_microphone \
    master=virtual_speaker.monitor \
    2>/dev/null || echo "[AUDIO] Virtual microphone already exists"

# Set defaults
pactl set-default-sink virtual_speaker 2>/dev/null || true
pactl set-default-source virtual_microphone 2>/dev/null || true

# Test audio configuration
if pactl list short sinks | grep -q virtual_speaker && \
   pactl list short sources | grep -q virtual_microphone; then
    echo "[AUDIO] ✅ Virtual audio devices configured successfully"
    
    # Show current configuration
    echo "[AUDIO] Current configuration:"
    echo "  Default Sink: $(pactl get-default-sink 2>/dev/null || echo 'unknown')"
    echo "  Default Source: $(pactl get-default-source 2>/dev/null || echo 'unknown')"
else
    echo "[AUDIO] ⚠ Virtual devices may not be fully configured"
fi

echo "[AUDIO] Audio environment setup complete"