#!/bin/bash

# Entrypoint script for Zoom Bot Container
# Sets up virtual audio devices and then executes the main command

echo "[CONTAINER] Starting Zoom Bot container setup..."

# Function to setup virtual audio devices
setup_virtual_audio() {
    echo "[AUDIO] Setting up virtual audio devices..."
    
    # Clean up any existing PulseAudio processes
    pkill pulseaudio 2>/dev/null || true
    sleep 1
    
    # Create runtime directory
    mkdir -p /tmp/pulse-runtime
    export PULSE_RUNTIME_PATH=/tmp/pulse-runtime
    
    # Start PulseAudio daemon with better configuration for containers
    echo "[AUDIO] Starting PulseAudio..."
    if [ "$(id -u)" = "0" ]; then
        # Running as root in container - use user mode but with explicit configuration
        export PULSE_SERVER=unix:/tmp/pulse-runtime/native
        pulseaudio --start --log-level=info --exit-idle-time=-1 &
    else
        # Running as regular user
        pulseaudio --start --log-level=info &
    fi
    
    # Wait for PulseAudio to be ready with longer timeout
    local attempts=0
    local max_attempts=15
    
    while [ $attempts -lt $max_attempts ]; do
        if pactl info >/dev/null 2>&1; then
            echo "[AUDIO] PulseAudio is ready"
            break
        fi
        echo "[AUDIO] Waiting for PulseAudio... (attempt $((attempts + 1))/$max_attempts)"
        sleep 2
        attempts=$((attempts + 1))
    done
    
    if [ $attempts -eq $max_attempts ]; then
        echo "[AUDIO] WARNING: PulseAudio startup timeout, trying alternative startup..."
        
        # Try system mode as fallback
        pulseaudio --system --disallow-exit --disallow-module-loading=false --daemonize --log-level=info || true
        sleep 2
        
        if ! pactl info >/dev/null 2>&1; then
            echo "[AUDIO] ERROR: Could not start PulseAudio, continuing without virtual audio"
            return 1
        fi
    fi
    
    # Create virtual audio devices
    echo "[AUDIO] Creating virtual speaker..."
    pactl load-module module-null-sink \
        sink_name=virtual_speaker \
        sink_properties=device.description=Virtual_Speaker \
        rate=44100 \
        channels=2 || echo "[AUDIO] Warning: Could not create virtual speaker"
    
    echo "[AUDIO] Creating virtual microphone..."
    pactl load-module module-virtual-source \
        source_name=virtual_microphone \
        rate=44100 \
        channels=2 || echo "[AUDIO] Warning: Could not create virtual microphone"
    
    # Set as defaults
    echo "[AUDIO] Setting virtual devices as defaults..."
    pactl set-default-sink virtual_speaker 2>/dev/null || true
    pactl set-default-source virtual_microphone 2>/dev/null || true
    
    echo "[AUDIO] Virtual audio setup complete!"
    
    # Show configuration
    if pactl info >/dev/null 2>&1; then
        echo "[AUDIO] Current configuration:"
        pactl info | grep -E "(Default Sink|Default Source)" | sed 's/^/  /'
        echo "[AUDIO] Available devices:"
        pactl list short sinks | sed 's/^/  Sink: /'
        pactl list short sources | sed 's/^/  Source: /'
    fi
}

# Setup virtual audio
setup_virtual_audio

echo "[CONTAINER] Container setup complete!"

# Execute the main command
if [ $# -eq 0 ]; then
    echo "[CONTAINER] No command specified, starting interactive shell..."
    exec /bin/bash
elif [ "$1" = "zoom_poc" ] || [ "$1" = "./zoom_poc" ]; then
    echo "[CONTAINER] Starting Zoom Bot with virtual audio setup..."
    exec /usr/local/bin/run-zoom-bot.sh "${@:2}"
else
    echo "[CONTAINER] Executing command: $@"
    exec "$@"
fi