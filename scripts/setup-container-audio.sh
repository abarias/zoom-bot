#!/bin/bash

# Container-Optimized PulseAudio Setup for Root User
# Fixes daemon stability issues in Docker containers

echo "[AUDIO] Configuring PulseAudio for container environment (root user)..."

# Kill any existing processes
pkill -f pulseaudio 2>/dev/null || true
sleep 2

# Clean up runtime directories
rm -rf /tmp/pulse-* /root/.pulse* /root/.config/pulse 2>/dev/null || true

# Create proper runtime directory
mkdir -p /tmp/pulse-runtime
chmod 755 /tmp/pulse-runtime

# Set environment variables for container
export PULSE_RUNTIME_PATH=/tmp/pulse-runtime
export PULSE_CONF_PATH=/etc/pulse
export PULSE_STATE_PATH=/tmp/pulse-runtime
export ALSA_PCM_CARD=pulse
export ALSA_PCM_DEVICE=0

# Create custom daemon configuration for root in container
cat > /tmp/pulse-daemon.conf << 'EOF'
# Container-optimized PulseAudio configuration
exit-idle-time = -1
flat-volumes = no
enable-shm = no
enable-memfd = no
high-priority = no
nice-level = 0
realtime-scheduling = no
system-instance = no
local-server-type = user
resample-method = trivial
default-sample-format = s16le
default-sample-rate = 44100
default-sample-channels = 2
default-channel-map = front-left,front-right
EOF

echo "[AUDIO] Starting PulseAudio daemon with container-optimized settings..."

# Start PulseAudio with custom config
pulseaudio \
    --system=false \
    --daemonize=true \
    --high-priority=false \
    --realtime=false \
    --disallow-module-loading=false \
    --disallow-exit=false \
    --exit-idle-time=-1 \
    --module-dir=/usr/lib/pulse-16.1/modules \
    --dl-search-path=/usr/lib/pulse-16.1/modules \
    --file=/tmp/pulse-daemon.conf \
    --log-level=error \
    --verbose=false \
    2>/dev/null

sleep 3

# Check if daemon is running
if pactl info >/dev/null 2>&1; then
    echo "[AUDIO] ✓ PulseAudio daemon started successfully"
else
    echo "[AUDIO] ⚠ Daemon not responding, trying system mode..."
    
    # Try system mode as fallback
    pulseaudio \
        --system \
        --daemonize \
        --high-priority=false \
        --realtime=false \
        --disallow-exit \
        --exit-idle-time=-1 \
        --log-level=error \
        2>/dev/null
    
    sleep 3
    
    if pactl info >/dev/null 2>&1; then
        echo "[AUDIO] ✓ PulseAudio started in system mode"
    else
        echo "[AUDIO] ❌ Could not start PulseAudio daemon"
        return 1
    fi
fi

# Load essential modules
echo "[AUDIO] Loading audio modules..."

# Load null sink (virtual speaker)
MODULE_ID_SPEAKER=$(pactl load-module module-null-sink \
    sink_name=virtual_speaker \
    sink_properties=device.description=Virtual_Speaker \
    rate=44100 channels=2 2>/dev/null)

if [ ! -z "$MODULE_ID_SPEAKER" ]; then
    echo "[AUDIO] ✓ Virtual speaker loaded (module: $MODULE_ID_SPEAKER)"
else
    echo "[AUDIO] ⚠ Could not load virtual speaker"
fi

# Load virtual source (virtual microphone)
MODULE_ID_MIC=$(pactl load-module module-virtual-source \
    source_name=virtual_microphone \
    master=virtual_speaker.monitor \
    rate=44100 channels=2 2>/dev/null)

if [ ! -z "$MODULE_ID_MIC" ]; then
    echo "[AUDIO] ✓ Virtual microphone loaded (module: $MODULE_ID_MIC)"
else
    echo "[AUDIO] ⚠ Could not load virtual microphone"
fi

# Set as defaults
pactl set-default-sink virtual_speaker 2>/dev/null || echo "[AUDIO] Could not set default sink"
pactl set-default-source virtual_microphone 2>/dev/null || echo "[AUDIO] Could not set default source"

# Verify setup
echo "[AUDIO] Verifying configuration..."
if pactl list short sinks | grep -q virtual_speaker && pactl list short sources | grep -q virtual_microphone; then
    echo "[AUDIO] ✅ Virtual audio devices configured successfully"
    
    echo "[AUDIO] Current devices:"
    pactl list short sinks | grep virtual
    pactl list short sources | grep virtual
    
    # Keep daemon alive by creating a persistent client
    pactl subscribe >/dev/null 2>&1 &
    SUBSCRIBE_PID=$!
    echo "[AUDIO] ✓ Daemon keepalive process: $SUBSCRIBE_PID"
    
    return 0
else
    echo "[AUDIO] ❌ Virtual devices not properly configured"
    return 1
fi