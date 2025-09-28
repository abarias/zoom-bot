#pragma once

#include <unordered_map>
#include <fstream>
#include <mutex>
#include <string>
#include <memory>
#include <cstdint>

// Zoom SDK raw data
#include "rawdata/zoom_rawdata_api.h"
#include "rawdata/rawdata_audio_helper_interface.h"
#include "zoom_sdk_raw_data_def.h"
#include "meeting_service_interface.h"
#include "meeting_service_components/meeting_audio_interface.h"
#include "meeting_service_components/meeting_participants_ctrl_interface.h"
#include "meeting_service_components/meeting_raw_archiving_interface.h"

// Our streaming system
#include "audio_streamer.h"

namespace ZoomBot {

// Simple PCM writer that opens a .pcm file and appends bytes
class PCMFile {
public:
    explicit PCMFile(const std::string& path);
    ~PCMFile();
    bool good() const;
    void write(const char* data, size_t len);
    void flush();
private:
    std::ofstream ofs_;
};

// Delegates raw audio frames to per-participant PCM files and streams to processing service
class AudioRawHandler : public ZOOM_SDK_NAMESPACE::IZoomSDKAudioRawDataDelegate {
public:
    AudioRawHandler();
    ~AudioRawHandler();

    // Start/stop subscription
    bool requestRecordingPermission();
    bool startRecording();
    bool stopRecording();
    bool subscribe(bool withInterpreters = false);
    void unsubscribe();
    void setMeetingService(ZOOM_SDK_NAMESPACE::IMeetingService* svc) { meetingService_ = svc; }
    
    // Streaming configuration
    bool enableStreaming(const std::string& backend_type = "tcp", 
                        const std::string& config = "localhost:8888");
    void disableStreaming();
    bool isStreamingEnabled() const { return streamer_ && streamer_->isConnected(); }
    
    // WAV conversion utility
    static bool convertPCMToWAV(const std::string& pcmFilePath, const std::string& wavFilePath, 
                                uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample = 16);
    void convertAllPCMToWAV() const;

    // IZoomSDKAudioRawDataDelegate
    void onMixedAudioRawDataReceived(AudioRawData* data_) override;
    void onOneWayAudioRawDataReceived(AudioRawData* data_, uint32_t user_id) override;
    void onShareAudioRawDataReceived(AudioRawData* data_, uint32_t user_id) override;
    void onOneWayInterpreterAudioRawDataReceived(AudioRawData* data_, const zchar_t* pLanguageName) override;

private:
    std::string outDir_;
    std::mutex mtx_;
    std::unique_ptr<PCMFile> mixedFile_;
    std::unordered_map<uint32_t, std::unique_ptr<PCMFile>> userFiles_;
    ZOOM_SDK_NAMESPACE::IMeetingService* meetingService_ = nullptr; // weak ref
    
    // Streaming system
    std::unique_ptr<AudioStreamer> streamer_;

    static bool ensureDir(const std::string& path);
    static std::string sanitize(const std::string& s);
    void writeToFile(PCMFile& file, AudioRawData* data_);
    void streamAudioData(uint32_t user_id, const std::string& user_name, AudioRawData* data_);
    std::string displayNameForUser(uint32_t user_id);
};

} // namespace ZoomBot
