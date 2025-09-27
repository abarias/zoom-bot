# Quick Test Guide - Once Raw Data is Enabled

## Build and Run
```bash
cmake --build /workspaces/zoom-bot/build -j 4
cd /workspaces/zoom-bot/build && ./zoom_poc
```

## Expected Output Flow

### 1. Raw Data License Check
```
[RAW DATA] Checking raw data license status...
[RAW DATA] HasRawdataLicense() = true
[RAW DATA] License confirmed! Requesting host permission...
```

### 2. Permission Request
```
[RAW DATA] Permission request sent to host successfully!  
[RAW DATA] Waiting for host approval (up to 30 seconds)...
```

### 3. Host Response (Success)
```
[CALLBACK] Raw data permission status: GRANTED - Raw data permission approved by host!
Host granted raw data permission! Proceeding with subscription.
```

### 4. Audio Subscription  
```
[AUDIO] Attempting to subscribe to raw audio data...
[AUDIO] ✓ Raw audio subscription successful!
[AUDIO] Starting per-participant audio capture...
```

### 5. File Creation
```
Bot is now in the meeting. Recording per-participant PCM in ./recordings. Press Ctrl+C to exit...
Created recording directory: ./recordings/20250124_153045
[AUDIO] Created file for user 12345 (JohnDoe): user_12345_JohnDoe_48000Hz_1ch.pcm
[AUDIO] Created file for user 67890 (JaneSmith): user_67890_JaneSmith_48000Hz_1ch.pcm  
```

## Host Permission Testing

### Host Actions Required:
1. When bot joins meeting, host will see permission request popup
2. Host must **approve** raw data recording request
3. Bot will receive callback and proceed with audio capture

### Timeout Scenario:
If host doesn't respond within 30 seconds:
```
[CALLBACK] Raw data permission status: TIMEOUT - Host did not respond to raw data permission request
Host has not yet responded to raw data permission request. Trying subscription anyway...
```

## Troubleshooting

### If Permission Denied:
```
[CALLBACK] Raw data permission status: DENIED - Raw data permission denied by host
```
→ Host needs to approve the request

### If Still Getting License Error:
```
[RAW DATA] HasRawdataLicense() = false
```
→ Raw Data feature not yet enabled in app marketplace

## File Verification

Check output directory:
```bash
ls -la recordings/*/
```

Play PCM files (if needed):
```bash
# Convert to WAV for playback
ffmpeg -f s16le -ar 48000 -ac 1 -i user_12345_JohnDoe_48000Hz_1ch.pcm output.wav
```

## Success Indicators

✅ **License**: `HasRawdataLicense() = true`  
✅ **Permission**: `GRANTED - Raw data permission approved by host!`
✅ **Subscription**: `Raw audio subscription successful!`
✅ **Files**: PCM files created in recordings directory