# Configuration

This directory contains configuration files for various system components:

## Audio Configuration
- `asound.conf` - ALSA audio system configuration
- `pulseaudio-virtual.pa` - PulseAudio virtual device configuration

## Docker Configuration
- `docker-compose.yml` - Docker Compose service definitions
- `docker-entrypoint.sh` - Container entry point script

## Usage
These configuration files are referenced by the main application and setup scripts. Most are automatically used when the system is properly configured through the setup scripts in the `scripts/` directory.