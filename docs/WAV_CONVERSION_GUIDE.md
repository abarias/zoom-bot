# WAV Conversion Feature for Zoom Bot

## Overview
The Zoom Bot now includes automatic PCM-to-WAV conversion functionality, making recorded audio files immediately playable in any standard audio player.

## Automatic Conversion
- **When**: Automatically triggered when recording is stopped (Ctrl+C)
- **What**: Converts all PCM files in the current session to WAV format
- **Format**: 16-bit PCM WAV files with original sample rate and channel configuration
- **Location**: WAV files created alongside PCM files in the same directory

## Manual Conversion Options

### 1. Standalone WAV Converter Utility
```bash
# Build the utility
make

# Convert a specific recording session
./wav_converter recordings/20250924_170906

# The utility will automatically detect and convert:
# - mixed_32000Hz_1ch.pcm → mixed_32000Hz_1ch.wav
# - user_16778240_Cory_Brightman_32000Hz_1ch.pcm → user_16778240_Cory_Brightman_32000Hz_1ch.wav
# - user_16784384_MyBot_32000Hz_1ch.pcm → user_16784384_MyBot_32000Hz_1ch.wav
```

### 2. Comprehensive Conversion Script
```bash
# Convert all recordings in a directory
./convert_to_wav.sh recordings

# Convert a specific session
./convert_to_wav.sh recordings/20250924_170906

# Features:
# - Automatically finds all PCM files
# - Provides progress feedback and file size information
# - Works with both single directories and batch processing
# - Includes playback instructions
```

## Technical Details

### WAV File Format
- **Encoding**: Uncompressed 16-bit PCM
- **Sample Rates**: Preserves original (typically 32000 Hz or 48000 Hz)
- **Channels**: Preserves original (1 or 2 channels)
- **Bit Depth**: 16-bit (standard for voice recordings)

### File Naming Convention
PCM files follow the pattern: `[type]_[info]_[sampleRate]Hz_[channels]ch.pcm`
WAV files mirror this: `[type]_[info]_[sampleRate]Hz_[channels]ch.wav`

Examples:
- `mixed_48000Hz_2ch.pcm` → `mixed_48000Hz_2ch.wav`
- `user_12345_JohnDoe_32000Hz_1ch.pcm` → `user_12345_JohnDoe_32000Hz_1ch.wav`

### WAV Header Structure
The converter creates standard WAV files with proper RIFF headers:
- RIFF chunk identifier
- File size information
- WAV format identifier
- fmt subchunk with audio format details
- data subchunk with actual audio samples

## Audio Quality
- **Lossless**: Direct PCM-to-WAV conversion with no quality loss
- **Compatibility**: Standard WAV format playable in all audio software
- **Efficiency**: Maintains original sample rates for optimal quality/size balance

## Playback Options

### Command Line Players
```bash
# VLC (if available)
vlc recordings/20250924_170906/*.wav

# MPV (lightweight)
mpv recordings/20250924_170906/mixed_32000Hz_1ch.wav

# ALSA player (basic)
aplay recordings/20250924_170906/mixed_32000Hz_1ch.wav

# FFmpeg player
ffplay recordings/20250924_170906/mixed_32000Hz_1ch.wav
```

### Analysis Tools
```bash
# Audio information
ffprobe recordings/20250924_170906/mixed_32000Hz_1ch.wav

# Convert to MP3 for sharing
ffmpeg -i mixed_32000Hz_1ch.wav -b:a 128k mixed_audio.mp3

# Extract specific time segment
ffmpeg -i mixed_32000Hz_1ch.wav -ss 00:01:30 -t 00:00:30 excerpt.wav
```

## Implementation Benefits

### For Development
- **Immediate Verification**: Quickly test recording quality
- **Debug Audio Issues**: Listen to individual participant streams
- **Quality Assessment**: Compare mixed vs individual audio

### For Production
- **User-Friendly**: No technical knowledge needed to play recordings
- **Universal Compatibility**: Works with any audio software
- **Archive Ready**: Standard format for long-term storage

### For Analysis
- **Participant Separation**: Individual WAV files for each speaker
- **Mixed Audio**: Combined stream for meeting overview
- **Timestamped**: Directory names include recording time

## File Size Considerations
- **PCM**: Raw audio data, larger files
- **WAV**: Adds ~44 bytes header, negligible size increase
- **Typical**: ~3.4MB for 30-second recording at 32kHz mono
- **Storage**: Consider converting to MP3 for archival if space is limited

## Error Handling
- **Missing Files**: Graceful handling of missing PCM files
- **Invalid Formats**: Automatic format detection with fallback defaults
- **Permission Issues**: Clear error messages for file access problems
- **Parsing Errors**: Robust filename parsing with sensible defaults

## Integration with Main Application
The WAV conversion is seamlessly integrated into the main recording workflow:

1. **Recording Active**: PCM files written continuously
2. **Stop Signal**: User presses Ctrl+C
3. **Stop Recording**: `StopRawRecording()` called
4. **Auto Convert**: `convertAllPCMToWAV()` automatically triggered
5. **Files Ready**: Both PCM and WAV files available for use

This ensures users always have immediately playable audio files without extra steps.