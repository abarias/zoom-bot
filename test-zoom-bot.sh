#!/bin/bash

# Quick test script for zoom bot with better error handling

echo "=== Quick Zoom Bot Test ==="

# Set up audio first
echo "[TEST] Setting up audio..."
if ! /workspaces/zoom-bot/setup-simple-audio.sh; then
    echo "[TEST] ❌ Audio setup failed"
    exit 1
fi

echo "[TEST] Starting zoom bot with 90 second timeout..."

# Run the bot with a longer timeout and better signal handling
timeout --preserve-status --kill-after=10s 90s /workspaces/zoom-bot/run-zoom-bot.sh

EXIT_CODE=$?

echo
echo "[TEST] Bot exited with code: $EXIT_CODE"

case $EXIT_CODE in
    0)   echo "[TEST] ✅ Bot completed successfully" ;;
    124) echo "[TEST] ⚠ Bot timed out after 90 seconds" ;;
    130) echo "[TEST] ⚠ Bot was interrupted (Ctrl+C)" ;;
    *)   echo "[TEST] ❌ Bot exited with error code $EXIT_CODE" ;;
esac

# Check if PulseAudio is still running
echo "[TEST] Audio status after bot exit:"
if pactl info >/dev/null 2>&1; then
    echo "  ✓ PulseAudio still running"
    pactl list short sinks | grep virtual && echo "  ✓ Virtual devices available"
else
    echo "  ⚠ PulseAudio not running"
fi