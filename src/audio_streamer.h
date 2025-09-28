#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <cstdint>

namespace ZoomBot {

// Forward declarations
struct AudioChunk;
class AudioStreamer;

/**
 * Abstract base class for audio streaming backends
 * This allows easy swapping between TCP, ZeroMQ, etc.
 */
class StreamingBackend {
public:
    virtual ~StreamingBackend() = default;
    virtual bool initialize(const std::string& config) = 0;
    virtual bool streamAudio(uint32_t user_id, const std::string& user_name, 
                           const char* data, size_t length, 
                           uint32_t sample_rate, uint16_t channels) = 0;
    virtual void shutdown() = 0;
};

/**
 * TCP-based streaming backend
 */
class TCPStreamingBackend : public StreamingBackend {
public:
    TCPStreamingBackend();
    ~TCPStreamingBackend() override;
    
    bool initialize(const std::string& config) override;
    bool streamAudio(uint32_t user_id, const std::string& user_name,
                    const char* data, size_t length,
                    uint32_t sample_rate, uint16_t channels) override;
    void shutdown() override;

private:
    struct TCPConnection {
        int socket_fd;
        std::string host;
        int port;
        bool connected;
    };
    
    std::unique_ptr<TCPConnection> connection_;
    std::mutex connection_mutex_;
    
    bool connectToServer();
    bool sendHeader(uint32_t user_id, const std::string& user_name, 
                   uint32_t sample_rate, uint16_t channels);
    bool sendAudioData(const char* data, size_t length);
};

/**
 * Audio chunk for queuing
 */
struct AudioChunk {
    uint32_t user_id;
    std::string user_name;
    std::vector<char> data;
    uint32_t sample_rate;
    uint16_t channels;
    
    AudioChunk(uint32_t id, const std::string& name, const char* audio_data, 
               size_t length, uint32_t rate, uint16_t ch)
        : user_id(id), user_name(name), data(audio_data, audio_data + length), 
          sample_rate(rate), channels(ch) {}
};

/**
 * Main audio streaming class that manages the backend and threading
 */
class AudioStreamer {
public:
    AudioStreamer();
    ~AudioStreamer();
    
    // Initialize with backend type and configuration
    bool initialize(const std::string& backend_type = "tcp", 
                   const std::string& config = "localhost:8888");
    
    // Queue audio data for streaming (non-blocking)
    void queueAudio(uint32_t user_id, const std::string& user_name,
                   const char* data, size_t length,
                   uint32_t sample_rate, uint16_t channels);
    
    // Start/stop streaming
    void start();
    void stop();
    
    // Stats
    size_t getQueueSize() const;
    bool isConnected() const;

private:
    std::unique_ptr<StreamingBackend> backend_;
    
    // Threading for async streaming
    std::thread worker_thread_;
    std::atomic<bool> running_;
    std::atomic<bool> connected_;
    
    // Audio queue
    std::queue<std::unique_ptr<AudioChunk>> audio_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // Worker thread function
    void workerLoop();
};

} // namespace ZoomBot