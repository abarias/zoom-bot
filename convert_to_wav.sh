#!/bin/bash

# WAV Converter Script for Zoom Bot Recordings
# This script converts all PCM files in a recordings directory to WAV format

echo "🎵 Zoom Bot Recording WAV Converter"
echo "=================================="

# Check if directory argument provided
if [ $# -eq 0 ]; then
    echo "Usage: $0 <recordings_directory>"
    echo "Example: $0 ./recordings/20250924_170906"
    echo "Or: $0 ./recordings  (to convert all subdirectories)"
    exit 1
fi

RECORDINGS_DIR="$1"

# Check if directory exists
if [ ! -d "$RECORDINGS_DIR" ]; then
    echo "❌ Error: Directory '$RECORDINGS_DIR' does not exist"
    exit 1
fi

echo "📁 Processing directory: $RECORDINGS_DIR"

# Function to convert PCM files in a directory
convert_directory() {
    local dir="$1"
    echo "🔄 Converting PCM files in: $dir"
    
    # Count PCM files
    pcm_count=$(find "$dir" -name "*.pcm" -type f | wc -l)
    
    if [ "$pcm_count" -eq 0 ]; then
        echo "⚠️  No PCM files found in $dir"
        return
    fi
    
    echo "📊 Found $pcm_count PCM files to convert"
    
    # Use our WAV converter utility
    if [ -x "./wav_converter" ]; then
        ./wav_converter "$dir"
    else
        echo "❌ Error: wav_converter utility not found. Please run 'make' first."
        return 1
    fi
    
    # Verify conversion
    wav_count=$(find "$dir" -name "*.wav" -type f | wc -l)
    echo "✅ Created $wav_count WAV files"
    
    # Show file sizes
    echo "📈 File summary:"
    ls -lh "$dir"/*.wav 2>/dev/null | awk '{print "   " $9 ": " $5}'
}

# Check if target is a single directory with PCM files or parent directory
if find "$RECORDINGS_DIR" -maxdepth 1 -name "*.pcm" -type f | grep -q .; then
    # Directory contains PCM files directly
    convert_directory "$RECORDINGS_DIR"
else
    # Check subdirectories for PCM files
    found_subdirs=0
    for subdir in "$RECORDINGS_DIR"/*/; do
        if [ -d "$subdir" ] && find "$subdir" -name "*.pcm" -type f | grep -q .; then
            convert_directory "$subdir"
            found_subdirs=$((found_subdirs + 1))
            echo ""
        fi
    done
    
    if [ "$found_subdirs" -eq 0 ]; then
        echo "⚠️  No PCM files found in '$RECORDINGS_DIR' or its subdirectories"
        echo "💡 Make sure you're pointing to a directory with recorded audio files"
    else
        echo "🎉 Completed conversion for $found_subdirs recording sessions"
    fi
fi

echo ""
echo "🎧 How to play your converted audio files:"
echo "   VLC:    vlc $RECORDINGS_DIR/*.wav"
echo "   MPV:    mpv $RECORDINGS_DIR/mixed_*.wav"
echo "   FFmpeg: ffplay $RECORDINGS_DIR/mixed_*.wav"
echo ""
echo "📝 Note: WAV files are now ready for analysis, editing, or playback!"