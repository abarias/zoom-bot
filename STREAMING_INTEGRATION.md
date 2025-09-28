# Audio Streaming Integration Guide

This document describes the new TCP-based audio streaming system that bridges the C++ Zoom Bot with Python audio processing.

## Overview

The system consists of three main components:

1. **C++ Audio Streamer** (`src/audio_streamer.*`) - TCP client that streams PCM audio data
2. **Python Audio Processor** (`audio_processor.py`) - TCP server that receives and processes audio
3. **Integration Layer** - Modified `AudioRawHandler` that feeds data to both file storage and streaming

## Architecture

```
Zoom SDK → AudioRawHandler → [File Storage] + [TCP Stream] → Python Service → WAV Files
                                                                           → [Future: Deepgram API]
```

### Data Flow

1. **Audio Capture**: C++ receives raw PCM audio from Zoom SDK per participant
2. **Dual Output**: Data is simultaneously written to local PCM files AND streamed via TCP
3. **Processing**: Python service receives streamed data and converts to WAV files
4. **Future**: Deepgram integration will replace WAV file writing with real-time transcription

## Protocol Specification

The TCP protocol uses a simple TLV (Type-Length-Value) format:

### Message Format
```
[Header Size: 4 bytes, network byte order]
[Header JSON: variable length UTF-8]
[Audio Data Size: 4 bytes, network byte order] 
[Audio Data: variable length PCM samples]
```

### Header JSON Structure
```json
{
  "type": "audio_header",
  "user_id": 12345,
  "user_name": "John_Doe", 
  "sample_rate": 32000,
  "channels": 1,
  "format": "pcm_s16le",
  "timestamp": 1627123456789
}
```

### Audio Data Format
- **Format**: PCM signed 16-bit little-endian
- **Sample Rate**: Typically 32kHz (varies by meeting settings)
- **Channels**: Usually 1 (mono) per participant
- **Byte Order**: Little-endian (standard PCM)

## Usage Instructions

### 1. Start the Python Audio Service

```bash
# Start the audio processor service
./start_audio_service.sh

# Or manually:
python3 audio_processor.py --host localhost --port 8888 --output-dir processed_audio
```

### 2. Run the Zoom Bot

The bot will automatically enable streaming when it successfully subscribes to audio data:

```bash
cd build
./zoom_poc
```

Look for these log messages:
```
[STREAMING] Enabling audio streaming...
[TCP] Configured to connect to localhost:8888
[TCP] ✓ Connected to audio processing server
[STREAMING] ✓ Audio streaming enabled!
```

### 3. Monitor Audio Processing

The Python service will log incoming audio:
```
📡 Client connected from ('127.0.0.1', 54321)
🎤 Started recording for John_Doe (ID: 12345)
📊 John_Doe: 10.1s recorded (645120 bytes)
```

### 4. Output Files

Processed WAV files will be saved to `processed_audio/`:
```
processed_audio/
├── user_12345_John_Doe_20240927_143022_32000Hz_1ch.wav
├── user_67890_Jane_Smith_20240927_143022_32000Hz_1ch.wav
└── mixed_audio_20240927_143022_32000Hz_1ch.wav
```

## Testing

### Test the TCP Protocol

Run the test script to verify the Python service:

```bash
# Ensure numpy is installed
pip3 install numpy

# Run test
python3 test_audio_processor.py
```

This sends synthetic audio data and should create test WAV files.

### Integration Test

1. Start the Python service: `./start_audio_service.sh`
2. Start the Zoom bot: `cd build && ./zoom_poc`
3. Join a meeting and verify streaming logs on both sides

## Configuration

### C++ Side (AudioRawHandler)

```cpp
// Enable streaming with custom backend and config
audioHandler.enableStreaming("tcp", "localhost:8888");

// Check streaming status
if (audioHandler.isStreamingEnabled()) {
    std::cout << "Streaming active" << std::endl;
}

// Disable streaming
audioHandler.disableStreaming();
```

### Python Side

```bash
# Custom host/port
python3 audio_processor.py --host 0.0.0.0 --port 9999

# Custom output directory
python3 audio_processor.py --output-dir /tmp/audio_output

# Verbose logging
python3 audio_processor.py --verbose
```

## Modular Design

The system is designed for easy extensibility:

### Adding New Backends (Future: ZeroMQ)

1. Implement new class inheriting from `StreamingBackend`
2. Add backend selection in `AudioStreamer::initialize()`
3. No changes needed to `AudioRawHandler`

Example:
```cpp
class ZeroMQStreamingBackend : public StreamingBackend {
    bool initialize(const std::string& config) override;
    bool streamAudio(...) override;
    void shutdown() override;
};
```

### Adding New Processing Modules

The Python service can be extended with processing plugins:

```python
class DeepgramProcessor:
    def process_audio_chunk(self, header, audio_data):
        # Send to Deepgram API
        # Return transcription
        pass

# Register processor
processor.add_module(DeepgramProcessor())
```

## Performance Considerations

- **Queue Size**: TCP backend limits queue to 1000 chunks (~50s of audio at 100ms chunks)
- **Memory Usage**: Each participant uses ~64KB buffer per second
- **Network**: ~64 KB/s per participant at 32kHz mono 16-bit PCM
- **Threading**: Audio streaming runs in separate worker thread to avoid blocking audio callbacks

## Troubleshooting

### "Failed to connect to audio processing server"
- Ensure Python service is running: `netstat -tln | grep 8888`
- Check firewall settings
- Verify host/port configuration matches

### "Audio streaming failed"
- Check if TCP service is accepting connections
- Verify no other process is using port 8888
- Check system resource limits (file descriptors, memory)

### "No audio data received"
- Ensure Zoom bot has recording permissions
- Check that bot successfully subscribed to raw audio data
- Verify bot is in meeting and VoIP is connected

### "WAV files are silent"
- Audio format mismatch - verify 16-bit PCM little-endian
- Check sample rate and channel configuration
- Verify audio data is not all zeros in logs

## Future Enhancements

1. **Deepgram Integration**: Replace WAV writing with real-time transcription
2. **WebSocket Support**: Add WebSocket backend for browser integration  
3. **Audio Enhancement**: Add noise reduction, echo cancellation
4. **Compression**: Add optional audio compression for network efficiency
5. **Load Balancing**: Support multiple processing service instances
6. **Monitoring**: Add metrics, health checks, and alerting

## File Structure

```
src/
├── audio_streamer.h          # Streaming system interface
├── audio_streamer.cpp        # TCP streaming implementation
├── audio_raw_handler.h       # Modified to include streaming
└── audio_raw_handler.cpp     # Integrated streaming calls

audio_processor.py            # Python TCP server service
test_audio_processor.py       # Protocol testing script  
start_audio_service.sh        # Service startup script
requirements.txt              # Updated Python dependencies
```