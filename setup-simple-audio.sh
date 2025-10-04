#!/bin/bash

# Ultra-Simple Audio Setup for Zoom Bot Container
# Focused on getting PulseAudio working reliably as root in container

echo "[AUDIO] Starting simple audio setup for container..."

# Clean slate
pkill -f pulseaudio 2>/dev/null || true
sleep 2
rm -rf /tmp/pulse* /root/.pulse* 2>/dev/null || true

# Create runtime directory
mkdir -p /tmp/pulse-runtime
chmod 755 /tmp/pulse-runtime

# Set basic environment
export PULSE_RUNTIME_PATH=/tmp/pulse-runtime
export ALSA_PCM_CARD=pulse

echo "[AUDIO] Starting PulseAudio..."

# Method 1: Try user mode first (with root warning suppression)
if pulseaudio --start --exit-idle-time=-1 --log-level=error >/dev/null 2>&1; then
    sleep 2
    if pactl info >/dev/null 2>&1; then
        echo "[AUDIO] ✓ PulseAudio started in user mode"
        PULSE_MODE="user"
    fi
fi

# Method 2: If user mode failed, try system mode
if [ -z "$PULSE_MODE" ]; then
    echo "[AUDIO] Trying system mode..."
    if pulseaudio --system --disallow-exit --exit-idle-time=-1 --log-level=error --daemonize >/dev/null 2>&1; then
        sleep 3
        if pactl info >/dev/null 2>&1; then
            echo "[AUDIO] ✓ PulseAudio started in system mode"
            PULSE_MODE="system"
        fi
    fi
fi

# Check if we have a working PulseAudio
if [ -z "$PULSE_MODE" ]; then
    echo "[AUDIO] ❌ Could not start PulseAudio in any mode"
    echo "[AUDIO] The bot will run without virtual audio (VoIP may fail)"
    exit 1
fi

echo "[AUDIO] Creating virtual audio devices..."

# Create null sink (virtual speaker) with error handling
SPEAKER_MODULE=$(pactl load-module module-null-sink \
    sink_name=virtual_speaker \
    sink_properties=device.description=Virtual_Speaker \
    rate=44100 channels=2 format=s16le 2>/dev/null)

if [ ! -z "$SPEAKER_MODULE" ]; then
    echo "[AUDIO] ✓ Virtual speaker created (module $SPEAKER_MODULE)"
else
    echo "[AUDIO] ⚠ Could not create virtual speaker, trying to find existing..."
    if pactl list short sinks | grep -q virtual_speaker; then
        echo "[AUDIO] ✓ Virtual speaker already exists"
    else
        echo "[AUDIO] ❌ No virtual speaker available"
    fi
fi

# Create virtual source (microphone)
MIC_MODULE=$(pactl load-module module-virtual-source \
    source_name=virtual_microphone \
    master=virtual_speaker.monitor 2>/dev/null)

if [ ! -z "$MIC_MODULE" ]; then
    echo "[AUDIO] ✓ Virtual microphone created (module $MIC_MODULE)"
else
    echo "[AUDIO] ⚠ Could not create virtual microphone, checking existing..."
    if pactl list short sources | grep -q virtual_microphone; then
        echo "[AUDIO] ✓ Virtual microphone already exists"
    else
        echo "[AUDIO] ❌ No virtual microphone available"
    fi
fi

# Set defaults
pactl set-default-sink virtual_speaker 2>/dev/null && echo "[AUDIO] ✓ Set default sink"
pactl set-default-source virtual_microphone 2>/dev/null && echo "[AUDIO] ✓ Set default source"

# Verify final setup
echo "[AUDIO] Final verification..."
if pactl list short sinks | grep -q virtual_speaker && pactl list short sources | grep -q virtual_microphone; then
    echo "[AUDIO] ✅ Virtual audio setup complete and verified!"
    
    # Show current devices
    echo "[AUDIO] Available sinks:"
    pactl list short sinks | grep virtual || echo "  (none found)"
    echo "[AUDIO] Available sources:"  
    pactl list short sources | grep virtual || echo "  (none found)"
    
    # Keep daemon alive with a background process
    (pactl subscribe > /dev/null 2>&1) &
    echo "[AUDIO] ✓ Daemon keepalive started"
    
    exit 0
else
    echo "[AUDIO] ❌ Virtual audio setup incomplete"
    echo "[AUDIO] Available sinks:"
    pactl list short sinks 2>/dev/null || echo "  (none)"
    echo "[AUDIO] Available sources:"
    pactl list short sources 2>/dev/null || echo "  (none)"
    
    exit 1
fi