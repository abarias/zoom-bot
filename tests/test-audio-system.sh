#!/bin/bash

echo "=== Audio System Test ==="
echo

# Test ALSA devices
echo "ALSA Devices:"
if command -v aplay >/dev/null 2>&1; then
    aplay -l 2>/dev/null || echo "  No ALSA hardware devices (expected in container)"
else
    echo "  aplay not available"
fi

echo
echo "ALSA PCM Devices:"
if command -v aplay >/dev/null 2>&1; then
    aplay -L 2>/dev/null | head -20 || echo "  Could not list PCM devices"
else
    echo "  aplay not available"  
fi

echo
echo "PulseAudio Status:"
if pactl info >/dev/null 2>&1; then
    echo "✅ PulseAudio is running"
    echo "Default Devices:"
    pactl info | grep -E "(Default Sink|Default Source)" | sed 's/^/  /'
else
    echo "❌ PulseAudio not accessible"
fi

echo
echo "Environment Variables:"
echo "  PULSE_RUNTIME_PATH: ${PULSE_RUNTIME_PATH:-not set}"
echo "  ALSA_PCM_CARD: ${ALSA_PCM_CARD:-not set}" 
echo "  ALSA_PCM_DEVICE: ${ALSA_PCM_DEVICE:-not set}"

echo
echo "ALSA Configuration:"
if [ -f /etc/asound.conf ]; then
    echo "✅ /etc/asound.conf exists"
else
    echo "❌ /etc/asound.conf missing"
fi

echo
echo "=== Test Complete ==="