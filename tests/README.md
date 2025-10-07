# Tests

This directory contains all test files and testing utilities:

## Test Scripts
- `test-audio-system.sh` - Audio system functionality tests
- `test-recording-permission.sh` - Recording permission validation
- `test-virtual-audio.sh` - Virtual audio device tests
- `test-zoom-bot.sh` - End-to-end bot testing
- `test_console_input.sh` - Console input validation

## Python Tests
- `test_audio_processor.py` - Audio processing service tests
- `test_connection.py` - TCP connection testing

## C++ Test Sources
- `test_auth.cpp` - Authentication testing
- `wav_converter.cpp` - WAV conversion utility
- `debug_test.cpp` - Debug functionality tests

## Running Tests
```bash
# Audio system tests
./tests/test-audio-system.sh

# Python service tests
python3 tests/test_audio_processor.py

# Build and run C++ tests
cd build && make test_auth && ./test_auth
```