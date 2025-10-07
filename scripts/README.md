# Scripts

This directory contains utility scripts for setting up and running the zoom bot:

## Setup Scripts
- `setup-env.sh` - Environment configuration setup
- `setup-container-audio.sh` - Container audio system setup
- `setup-robust-audio.sh` - Robust audio configuration
- `setup-simple-audio.sh` - Simple audio setup
- `setup-virtual-audio.sh` - Virtual audio device setup

## Utility Scripts
- `start_audio_service.sh` - Start the Python audio processing service
- `convert_to_wav.sh` - Convert PCM recordings to WAV format
- `glib_implementation_guide.sh` - GLib setup automation

## Usage
Most scripts can be run from the project root:
```bash
./scripts/setup-env.sh
./scripts/start_audio_service.sh
```