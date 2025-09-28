#include "audio_streamer.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <chrono>
#include <nlohmann/json.hpp>

namespace ZoomBot {

// ============================================================================
// TCPStreamingBackend Implementation
// ============================================================================

TCPStreamingBackend::TCPStreamingBackend() 
    : connection_(std::make_unique<TCPConnection>()) {
    connection_->socket_fd = -1;
    connection_->connected = false;
}

TCPStreamingBackend::~TCPStreamingBackend() {
    shutdown();
}

bool TCPStreamingBackend::initialize(const std::string& config) {
    // Parse config: "host:port"
    size_t colon_pos = config.find(':');
    if (colon_pos == std::string::npos) {
        std::cerr << "[TCP] Invalid config format. Expected 'host:port', got: " << config << std::endl;
        return false;
    }
    
    connection_->host = config.substr(0, colon_pos);
    connection_->port = std::stoi(config.substr(colon_pos + 1));
    
    std::cout << "[TCP] Configured to connect to " << connection_->host 
              << ":" << connection_->port << std::endl;
    
    return connectToServer();
}

bool TCPStreamingBackend::connectToServer() {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    
    // Close existing connection
    if (connection_->socket_fd != -1) {
        close(connection_->socket_fd);
        connection_->socket_fd = -1;
        connection_->connected = false;
    }
    
    // Create socket
    connection_->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (connection_->socket_fd < 0) {
        std::cerr << "[TCP] Failed to create socket" << std::endl;
        return false;
    }
    
    // Set up server address
    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(connection_->port);
    
    if (inet_pton(AF_INET, connection_->host.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "[TCP] Invalid address: " << connection_->host << std::endl;
        close(connection_->socket_fd);
        connection_->socket_fd = -1;
        return false;
    }
    
    // Connect to server
    if (connect(connection_->socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[TCP] Failed to connect to " << connection_->host 
                  << ":" << connection_->port << " - " << strerror(errno) << std::endl;
        close(connection_->socket_fd);
        connection_->socket_fd = -1;
        return false;
    }
    
    connection_->connected = true;
    std::cout << "[TCP] ✓ Connected to audio processing server" << std::endl;
    return true;
}

bool TCPStreamingBackend::streamAudio(uint32_t user_id, const std::string& user_name,
                                    const char* data, size_t length,
                                    uint32_t sample_rate, uint16_t channels) {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    
    if (!connection_->connected || connection_->socket_fd < 0) {
        // Try to reconnect
        if (!connectToServer()) {
            return false;
        }
    }
    
    // Send header first (JSON metadata)
    if (!sendHeader(user_id, user_name, sample_rate, channels)) {
        return false;
    }
    
    // Send audio data
    if (!sendAudioData(data, length)) {
        return false;
    }
    
    return true;
}

bool TCPStreamingBackend::sendHeader(uint32_t user_id, const std::string& user_name,
                                   uint32_t sample_rate, uint16_t channels) {
    nlohmann::json header;
    header["type"] = "audio_header";
    header["user_id"] = user_id;
    header["user_name"] = user_name;
    header["sample_rate"] = sample_rate;
    header["channels"] = channels;
    header["format"] = "pcm_s16le";
    header["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    std::string header_str = header.dump();
    uint32_t header_size = htonl(static_cast<uint32_t>(header_str.size()));
    
    // Send header size (4 bytes, network byte order)
    if (send(connection_->socket_fd, &header_size, sizeof(header_size), 0) != sizeof(header_size)) {
        std::cerr << "[TCP] Failed to send header size" << std::endl;
        connection_->connected = false;
        return false;
    }
    
    // Send header JSON
    if (send(connection_->socket_fd, header_str.c_str(), header_str.size(), 0) != static_cast<ssize_t>(header_str.size())) {
        std::cerr << "[TCP] Failed to send header" << std::endl;
        connection_->connected = false;
        return false;
    }
    
    return true;
}

bool TCPStreamingBackend::sendAudioData(const char* data, size_t length) {
    uint32_t data_size = htonl(static_cast<uint32_t>(length));
    
    // Send data size (4 bytes, network byte order)
    if (send(connection_->socket_fd, &data_size, sizeof(data_size), 0) != sizeof(data_size)) {
        std::cerr << "[TCP] Failed to send data size" << std::endl;
        connection_->connected = false;
        return false;
    }
    
    // Send audio data
    size_t bytes_sent = 0;
    while (bytes_sent < length) {
        ssize_t sent = send(connection_->socket_fd, data + bytes_sent, length - bytes_sent, 0);
        if (sent <= 0) {
            std::cerr << "[TCP] Failed to send audio data" << std::endl;
            connection_->connected = false;
            return false;
        }
        bytes_sent += sent;
    }
    
    return true;
}

void TCPStreamingBackend::shutdown() {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    
    if (connection_->socket_fd != -1) {
        close(connection_->socket_fd);
        connection_->socket_fd = -1;
    }
    connection_->connected = false;
    
    std::cout << "[TCP] Connection closed" << std::endl;
}

// ============================================================================
// AudioStreamer Implementation
// ============================================================================

AudioStreamer::AudioStreamer() 
    : running_(false), connected_(false) {}

AudioStreamer::~AudioStreamer() {
    stop();
}

bool AudioStreamer::initialize(const std::string& backend_type, const std::string& config) {
    if (backend_type == "tcp") {
        backend_ = std::make_unique<TCPStreamingBackend>();
    } else {
        std::cerr << "[STREAMER] Unsupported backend type: " << backend_type << std::endl;
        return false;
    }
    
    if (!backend_->initialize(config)) {
        std::cerr << "[STREAMER] Failed to initialize backend" << std::endl;
        backend_.reset();
        return false;
    }
    
    connected_.store(true);
    std::cout << "[STREAMER] ✓ Initialized " << backend_type << " streaming backend" << std::endl;
    return true;
}

void AudioStreamer::queueAudio(uint32_t user_id, const std::string& user_name,
                              const char* data, size_t length,
                              uint32_t sample_rate, uint16_t channels) {
    if (!backend_ || !running_.load()) {
        return;
    }
    
    auto chunk = std::make_unique<AudioChunk>(user_id, user_name, data, length, sample_rate, channels);
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        audio_queue_.push(std::move(chunk));
        
        // Prevent queue from growing too large (drop old data)
        const size_t MAX_QUEUE_SIZE = 1000;
        while (audio_queue_.size() > MAX_QUEUE_SIZE) {
            std::cout << "[STREAMER] Warning: Queue overflow, dropping old audio data" << std::endl;
            audio_queue_.pop();
        }
    }
    
    queue_cv_.notify_one();
}

void AudioStreamer::start() {
    if (running_.load() || !backend_) {
        return;
    }
    
    running_.store(true);
    worker_thread_ = std::thread(&AudioStreamer::workerLoop, this);
    
    std::cout << "[STREAMER] ✓ Started audio streaming worker thread" << std::endl;
}

void AudioStreamer::stop() {
    if (!running_.load()) {
        return;
    }
    
    std::cout << "[STREAMER] Stopping audio streamer..." << std::endl;
    running_.store(false);
    queue_cv_.notify_all();
    
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    
    if (backend_) {
        backend_->shutdown();
    }
    
    // Clear queue
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        while (!audio_queue_.empty()) {
            audio_queue_.pop();
        }
    }
    
    connected_.store(false);
    std::cout << "[STREAMER] ✓ Audio streamer stopped" << std::endl;
}

void AudioStreamer::workerLoop() {
    std::cout << "[STREAMER] Worker thread started" << std::endl;
    
    while (running_.load()) {
        std::unique_ptr<AudioChunk> chunk;
        
        // Get next chunk from queue
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] { 
                return !audio_queue_.empty() || !running_.load(); 
            });
            
            if (!running_.load()) {
                break;
            }
            
            if (!audio_queue_.empty()) {
                chunk = std::move(audio_queue_.front());
                audio_queue_.pop();
            }
        }
        
        // Process chunk
        if (chunk && backend_) {
            bool success = backend_->streamAudio(
                chunk->user_id, chunk->user_name,
                chunk->data.data(), chunk->data.size(),
                chunk->sample_rate, chunk->channels
            );
            
            if (!success) {
                std::cerr << "[STREAMER] Failed to stream audio for user " 
                          << chunk->user_id << " (" << chunk->user_name << ")" << std::endl;
                connected_.store(false);
                
                // Try to reconnect after a short delay
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                if (backend_->initialize("localhost:8888")) { // TODO: store config
                    connected_.store(true);
                }
            } else {
                connected_.store(true);
            }
        }
    }
    
    std::cout << "[STREAMER] Worker thread finished" << std::endl;
}

size_t AudioStreamer::getQueueSize() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return audio_queue_.size();
}

bool AudioStreamer::isConnected() const {
    return connected_.load();
}

} // namespace ZoomBot