#!/bin/bash

# Start Audio Processing Service for Zoom Bot
# This script starts the Python audio processor service

set -e

echo "🎵 Starting Zoom Bot Audio Processing Service"

# Check if Python requirements are installed
echo "📦 Checking Python dependencies..."
if ! python3 -c "import numpy, json, socket, wave, threading" > /dev/null 2>&1; then
    echo "⚠ Installing Python dependencies..."
    pip3 install -r requirements.txt
fi

# Create output directory
mkdir -p processed_audio

# Start the audio processor
echo "🚀 Starting audio processor on localhost:8888"
echo "📁 Output directory: processed_audio/"
echo ""
echo "To test, run in another terminal:"
echo "  python3 test_audio_processor.py"
echo ""
echo "To stop the service, press Ctrl+C"
echo ""

python3 audio_processor.py --host localhost --port 8888 --output-dir processed_audio --verbose