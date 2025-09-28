#!/usr/bin/env python3
"""Simple test to verify TCP connection to audio processor"""
import socket

try:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(5)
    result = sock.connect_ex(('localhost', 8888))
    if result == 0:
        print("✅ Successfully connected to audio processor on localhost:8888")
        print("🎵 The streaming integration is ready!")
        sock.close()
    else:
        print("❌ Could not connect to audio processor")
        print(f"Connection result: {result}")
except Exception as e:
    print(f"❌ Connection error: {e}")