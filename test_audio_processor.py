#!/usr/bin/env python3
"""
Test script to verify the audio processor service
Sends fake audio data to test the TCP protocol
"""

import socket
import json
import struct
import time
import random
import numpy as np

def generate_test_audio(duration_seconds=2, sample_rate=32000, channels=1):
    """Generate test sine wave audio data"""
    samples = int(duration_seconds * sample_rate)
    frequency = 440.0  # A4 note
    
    t = np.linspace(0, duration_seconds, samples, False)
    sine_wave = np.sin(2 * np.pi * frequency * t)
    
    # Convert to 16-bit PCM
    audio_data = (sine_wave * 32767).astype(np.int16).tobytes()
    return audio_data

def send_test_stream(host="localhost", port=8888):
    """Send test audio stream to processor"""
    
    # Connect to processor
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect((host, port))
        print(f"✓ Connected to audio processor at {host}:{port}")
        
        # Simulate multiple participants
        participants = [
            {"user_id": 12345, "user_name": "Test_User_Alice"},
            {"user_id": 67890, "user_name": "Test_User_Bob"},
            {"user_id": 0, "user_name": "Mixed_Audio"}  # Mixed audio stream
        ]
        
        for i in range(10):  # Send 10 chunks per participant
            for participant in participants:
                # Generate test audio chunk (0.5 seconds)
                audio_data = generate_test_audio(0.5)
                
                # Create header
                header = {
                    "type": "audio_header",
                    "user_id": participant["user_id"],
                    "user_name": participant["user_name"],
                    "sample_rate": 32000,
                    "channels": 1,
                    "format": "pcm_s16le",
                    "timestamp": int(time.time() * 1000)
                }
                
                header_json = json.dumps(header).encode('utf-8')
                
                # Send header size
                sock.send(struct.pack('!I', len(header_json)))
                
                # Send header
                sock.send(header_json)
                
                # Send audio data size
                sock.send(struct.pack('!I', len(audio_data)))
                
                # Send audio data
                sock.send(audio_data)
                
                print(f"📤 Sent 0.5s audio chunk for {participant['user_name']} (chunk {i+1}/10)")
            
            time.sleep(0.1)  # Small delay between chunks
        
        print("✅ Test stream complete!")
        
    except Exception as e:
        print(f"❌ Error: {e}")
    finally:
        sock.close()

if __name__ == '__main__':
    print("🧪 Testing Audio Processor Service")
    send_test_stream()