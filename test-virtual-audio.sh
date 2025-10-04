#!/bin/bash

# Test script to verify virtual audio setup for Zoom Bot

echo "=== Zoom Bot Virtual Audio Test ==="
echo

# Check if PulseAudio is running
if pgrep -x "pulseaudio" > /dev/null; then
    echo "✅ PulseAudio is running"
else
    echo "❌ PulseAudio is not running"
    exit 1
fi

# Check if pactl is working
if pactl info >/dev/null 2>&1; then
    echo "✅ PulseAudio control is working"
else
    echo "❌ Cannot communicate with PulseAudio"
    exit 1
fi

echo
echo "=== Audio Device Information ==="
echo

# Show default devices
echo "Default Audio Devices:"
pactl info | grep -E "(Default Sink|Default Source)" | sed 's/^/  /'

echo
echo "Available Sinks (Output Devices):"
pactl list short sinks | sed 's/^/  /'

echo
echo "Available Sources (Input Devices):"
pactl list short sources | sed 's/^/  /'

echo
echo "=== Virtual Device Check ==="

# Check for virtual devices
VIRTUAL_SINK=$(pactl list short sinks | grep "virtual_speaker" | wc -l)
VIRTUAL_SOURCE=$(pactl list short sources | grep "virtual_microphone" | wc -l)
VIRTUAL_MONITOR=$(pactl list short sources | grep "virtual_speaker.monitor" | wc -l)

if [ $VIRTUAL_SINK -gt 0 ]; then
    echo "✅ Virtual speaker (sink) is available"
else
    echo "❌ Virtual speaker (sink) is missing"
fi

if [ $VIRTUAL_SOURCE -gt 0 ]; then
    echo "✅ Virtual microphone (source) is available"
else
    echo "❌ Virtual microphone (source) is missing"
fi

if [ $VIRTUAL_MONITOR -gt 0 ]; then
    echo "✅ Virtual speaker monitor is available"
else
    echo "❌ Virtual speaker monitor is missing"
fi

# Check defaults
DEFAULT_SINK=$(pactl info | grep "Default Sink:" | awk '{print $3}')
DEFAULT_SOURCE=$(pactl info | grep "Default Source:" | awk '{print $3}')

if [ "$DEFAULT_SINK" = "virtual_speaker" ]; then
    echo "✅ Virtual speaker is set as default output"
else
    echo "⚠️  Default sink is: $DEFAULT_SINK (expected: virtual_speaker)"
fi

if [ "$DEFAULT_SOURCE" = "virtual_microphone" ]; then
    echo "✅ Virtual microphone is set as default input"
else
    echo "⚠️  Default source is: $DEFAULT_SOURCE (expected: virtual_microphone)"
fi

echo
echo "=== Audio Test Summary ==="

if [ $VIRTUAL_SINK -gt 0 ] && [ $VIRTUAL_SOURCE -gt 0 ]; then
    echo "✅ Virtual audio setup is ready for Zoom Bot!"
    echo "   The bot should be able to join meetings with audio capabilities."
else
    echo "❌ Virtual audio setup is incomplete."
    echo "   Run './setup-virtual-audio.sh' to fix the configuration."
fi

echo
echo "=== Commands to manually fix audio if needed ==="
echo "  pactl load-module module-null-sink sink_name=virtual_speaker"
echo "  pactl load-module module-virtual-source source_name=virtual_microphone"
echo "  pactl set-default-sink virtual_speaker"
echo "  pactl set-default-source virtual_microphone"
echo