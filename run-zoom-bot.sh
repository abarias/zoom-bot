#!/bin/bash

# Zoom Bot Launcher with Virtual Audio Setup
# This script ensures virtual audio devices are available before starting the bot

echo "=== Zoom Bot Launcher ==="
echo

# Function to start PulseAudio reliably
start_pulseaudio() {
    echo "[SETUP] Starting PulseAudio for virtual audio..."
    
    # Kill any existing processes
    pkill pulseaudio 2>/dev/null || true
    sleep 1
    
    # Try different startup methods
    if pulseaudio --start --log-level=info --verbose 2>/dev/null; then
        sleep 2
        if pactl info >/dev/null 2>&1; then
            echo "[SETUP] ✓ PulseAudio started successfully"
            return 0
        fi
    fi
    
    echo "[SETUP] Retrying with system mode..."
    if pulseaudio --system --disallow-exit --disallow-module-loading=false --daemonize --log-level=info 2>/dev/null; then
        sleep 2
        if pactl info >/dev/null 2>&1; then
            echo "[SETUP] ✓ PulseAudio started in system mode"
            return 0
        fi
    fi
    
    return 1
}

# Function to create virtual devices
create_virtual_devices() {
    echo "[SETUP] Creating virtual audio devices..."
    
    # Create virtual speaker
    if pactl load-module module-null-sink sink_name=virtual_speaker sink_properties=device.description=Virtual_Speaker >/dev/null 2>&1; then
        echo "[SETUP] ✓ Virtual speaker created"
    else
        echo "[SETUP] ⚠ Could not create virtual speaker"
    fi
    
    # Create virtual microphone  
    if pactl load-module module-virtual-source source_name=virtual_microphone >/dev/null 2>&1; then
        echo "[SETUP] ✓ Virtual microphone created"
    else
        echo "[SETUP] ⚠ Could not create virtual microphone"
    fi
    
    # Set as defaults
    pactl set-default-sink virtual_speaker 2>/dev/null || true
    pactl set-default-source virtual_microphone 2>/dev/null || true
    
    echo "[SETUP] ✓ Virtual audio devices configured"
}

# Function to verify setup
verify_audio_setup() {
    echo "[SETUP] Verifying audio configuration..."
    
    if pactl info >/dev/null 2>&1; then
        echo "[SETUP] ✓ PulseAudio accessible"
        
        local sink_count=$(pactl list short sinks | grep virtual_speaker | wc -l)
        local source_count=$(pactl list short sources | grep virtual_microphone | wc -l)
        
        if [ $sink_count -gt 0 ] && [ $source_count -gt 0 ]; then
            echo "[SETUP] ✓ Virtual devices available"
            return 0
        else
            echo "[SETUP] ⚠ Virtual devices missing"
            return 1
        fi
    else
        echo "[SETUP] ❌ PulseAudio not accessible"
        return 1
    fi
}

# Main setup process
echo "[SETUP] Preparing virtual audio environment for Zoom Bot..."

# Use simple audio setup that actually works
if [ -f "/workspaces/zoom-bot/scripts/setup-simple-audio.sh" ]; then
    echo "[SETUP] Using container-optimized audio setup..."
    if /workspaces/zoom-bot/scripts/setup-simple-audio.sh; then
        echo "[SETUP] ✅ Audio environment configured successfully!"
    else
        echo "[SETUP] ⚠ Audio setup had issues but will continue"
    fi
else
    echo "[SETUP] ⚠ Audio setup script not found, bot may have VoIP issues"
fi

echo
echo "=== Starting Zoom Bot ==="

# Load environment variables
if [ -f "/workspaces/zoom-bot/.env.local" ]; then
    echo "[SETUP] Loading environment variables from .env.local..."
    source /workspaces/zoom-bot/.env.local
    echo "[SETUP] ✓ Environment variables loaded"
else
    echo "[SETUP] ⚠ .env.local not found - you may need to set Zoom credentials manually"
fi

# Set audio environment variables
export PULSE_RUNTIME_PATH=/tmp/pulse-runtime
export ALSA_PCM_CARD=pulse
export ALSA_PCM_DEVICE=0

# Change to build directory and run the bot
cd /workspaces/zoom-bot/build

# Pass all arguments to the zoom bot
exec ./zoom_poc "$@"