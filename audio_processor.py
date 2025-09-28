#!/usr/bin/env python3
"""
Audio Processing Service

Receives streaming PCM audio data from the C++ Zoom Bot via TCP,
processes it, and outputs WAV files (will later be replaced with Deepgram streaming).

This service is designed to be easily modular for future integration with
Deepgram or other transcription services.

Protocol:
- Each message starts with 4-byte header size (network byte order)
- Header is JSON with metadata (user_id, user_name, sample_rate, channels, format, timestamp)
- Then 4-byte audio data size (network byte order)  
- Then raw PCM audio data
"""

import socket
import json
import struct
import threading
import wave
import os
from pathlib import Path
from typing import Dict, Optional, BinaryIO
import logging
from datetime import datetime
import argparse

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('AudioProcessor')


class AudioBuffer:
    """Manages buffered WAV writing for a single participant"""
    
    def __init__(self, user_id: int, user_name: str, sample_rate: int, channels: int, output_dir: Path):
        self.user_id = user_id
        self.user_name = user_name
        self.sample_rate = sample_rate
        self.channels = channels
        self.output_dir = output_dir
        self.bytes_written = 0
        self.wav_file: Optional[BinaryIO] = None
        self.start_time = datetime.now()
        
        # Create output file
        self._create_wav_file()
    
    def _create_wav_file(self):
        """Create WAV file with proper header"""
        timestamp = self.start_time.strftime("%Y%m%d_%H%M%S")
        
        if self.user_id == 0:
            filename = f"mixed_audio_{timestamp}_{self.sample_rate}Hz_{self.channels}ch.wav"
        else:
            safe_name = "".join(c if c.isalnum() or c in '-_' else '_' for c in self.user_name)
            filename = f"user_{self.user_id}_{safe_name}_{timestamp}_{self.sample_rate}Hz_{self.channels}ch.wav"
        
        self.output_path = self.output_dir / filename
        logger.info(f"Creating WAV file: {self.output_path}")
        
        # Open WAV file for writing
        self.wav_file = wave.open(str(self.output_path), 'wb')
        self.wav_file.setnchannels(self.channels)
        self.wav_file.setsampwidth(2)  # 16-bit PCM
        self.wav_file.setframerate(self.sample_rate)
    
    def write_audio_data(self, data: bytes):
        """Write PCM audio data to WAV file"""
        if self.wav_file:
            self.wav_file.writeframes(data)
            self.bytes_written += len(data)
    
    def close(self):
        """Close WAV file"""
        if self.wav_file:
            self.wav_file.close()
            self.wav_file = None
            
            duration = self.bytes_written / (self.sample_rate * self.channels * 2)  # 16-bit = 2 bytes
            logger.info(f"Closed WAV file: {self.output_path} ({duration:.2f}s, {self.bytes_written} bytes)")


class AudioProcessor:
    """Main audio processing service"""
    
    def __init__(self, host: str = "localhost", port: int = 8888, output_dir: str = "processed_audio"):
        self.host = host
        self.port = port
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(exist_ok=True)
        
        self.running = False
        self.server_socket = None
        self.audio_buffers: Dict[int, AudioBuffer] = {}
        self.client_threads = []
        
        logger.info(f"Audio processor initialized - listening on {host}:{port}")
        logger.info(f"Output directory: {self.output_dir.absolute()}")
    
    def start(self):
        """Start the audio processing server"""
        self.running = True
        
        # Create server socket
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        try:
            self.server_socket.bind((self.host, self.port))
            self.server_socket.listen(5)
            logger.info(f"🎵 Audio processing server started on {self.host}:{self.port}")
            
            while self.running:
                try:
                    client_socket, client_address = self.server_socket.accept()
                    logger.info(f"📡 Client connected from {client_address}")
                    
                    # Handle client in separate thread
                    client_thread = threading.Thread(
                        target=self._handle_client,
                        args=(client_socket, client_address),
                        daemon=True
                    )
                    client_thread.start()
                    self.client_threads.append(client_thread)
                    
                except OSError:
                    if self.running:
                        logger.error("Server socket error")
                    break
                    
        except Exception as e:
            logger.error(f"Server error: {e}")
        finally:
            self._cleanup()
    
    def stop(self):
        """Stop the audio processing server"""
        logger.info("Stopping audio processing server...")
        self.running = False
        
        if self.server_socket:
            self.server_socket.close()
        
        self._cleanup()
    
    def _cleanup(self):
        """Clean up resources"""
        # Close all audio buffers
        for buffer in self.audio_buffers.values():
            buffer.close()
        self.audio_buffers.clear()
        
        logger.info("✅ Audio processing server stopped")
    
    def _handle_client(self, client_socket: socket.socket, client_address):
        """Handle individual client connection"""
        try:
            while self.running:
                # Read header size (4 bytes, network byte order)
                header_size_data = self._recv_exact(client_socket, 4)
                if not header_size_data:
                    break
                
                header_size = struct.unpack('!I', header_size_data)[0]
                
                # Read header JSON
                header_data = self._recv_exact(client_socket, header_size)
                if not header_data:
                    break
                
                try:
                    header = json.loads(header_data.decode('utf-8'))
                except json.JSONDecodeError as e:
                    logger.error(f"Invalid JSON header: {e}")
                    continue
                
                # Read audio data size (4 bytes, network byte order)
                data_size_data = self._recv_exact(client_socket, 4)
                if not data_size_data:
                    break
                
                data_size = struct.unpack('!I', data_size_data)[0]
                
                # Read audio data
                audio_data = self._recv_exact(client_socket, data_size)
                if not audio_data:
                    break
                
                # Process the audio data
                self._process_audio_chunk(header, audio_data)
                
        except Exception as e:
            logger.error(f"Client handler error: {e}")
        finally:
            client_socket.close()
            logger.info(f"📡 Client {client_address} disconnected")
    
    def _recv_exact(self, sock: socket.socket, size: int) -> Optional[bytes]:
        """Receive exactly 'size' bytes from socket"""
        data = b''
        while len(data) < size:
            chunk = sock.recv(size - len(data))
            if not chunk:
                return None
            data += chunk
        return data
    
    def _process_audio_chunk(self, header: dict, audio_data: bytes):
        """Process a single audio chunk"""
        user_id = header.get('user_id', 0)
        user_name = header.get('user_name', f'User_{user_id}')
        sample_rate = header.get('sample_rate', 32000)
        channels = header.get('channels', 1)
        
        # Get or create audio buffer for this user
        if user_id not in self.audio_buffers:
            self.audio_buffers[user_id] = AudioBuffer(
                user_id, user_name, sample_rate, channels, self.output_dir
            )
            logger.info(f"🎤 Started recording for {user_name} (ID: {user_id})")
        
        # Write audio data to buffer
        buffer = self.audio_buffers[user_id]
        buffer.write_audio_data(audio_data)
        
        # Log progress periodically
        if buffer.bytes_written % (sample_rate * channels * 2 * 10) < len(audio_data):  # Every ~10 seconds
            duration = buffer.bytes_written / (sample_rate * channels * 2)
            logger.info(f"📊 {user_name}: {duration:.1f}s recorded ({buffer.bytes_written} bytes)")


def main():
    parser = argparse.ArgumentParser(description='Audio Processing Service for Zoom Bot')
    parser.add_argument('--host', default='localhost', help='Host to bind to')
    parser.add_argument('--port', type=int, default=8888, help='Port to bind to')
    parser.add_argument('--output-dir', default='processed_audio', help='Output directory for WAV files')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose logging')
    
    args = parser.parse_args()
    
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    
    processor = AudioProcessor(args.host, args.port, args.output_dir)
    
    try:
        processor.start()
    except KeyboardInterrupt:
        logger.info("Received Ctrl+C, shutting down...")
        processor.stop()


if __name__ == '__main__':
    main()