#include "audio_raw_handler.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <cctype>
#include <thread>
#include <dirent.h>
#include <chrono>

namespace ZoomBot {

// ---------------- PCMFile ----------------
PCMFile::PCMFile(const std::string& path) : ofs_(path, std::ios::binary | std::ios::out | std::ios::app) {}
PCMFile::~PCMFile() { if (ofs_.is_open()) ofs_.close(); }
bool PCMFile::good() const { return ofs_.good(); }
void PCMFile::write(const char* data, size_t len) { ofs_.write(data, static_cast<std::streamsize>(len)); }
void PCMFile::flush() { ofs_.flush(); }

// --------------- AudioRawHandler ---------------
static std::string timestampForFile() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_r(&t, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm);
    return buf;
}

bool AudioRawHandler::ensureDir(const std::string& path) {
    struct stat st{};
    if (stat(path.c_str(), &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return mkdir(path.c_str(), 0755) == 0;
}

std::string AudioRawHandler::sanitize(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' ) out.push_back(c);
        else out.push_back('_');
    }
    return out;
}

static std::string buildUserFilenameInDir(const std::string& dir, uint32_t user_id, unsigned int sampleRate, unsigned int channels) {
    std::ostringstream oss;
    oss << dir << "/user_" << user_id
        << "_" << sampleRate << "Hz_" << channels << "ch.pcm";
    return oss.str();
}

static std::string buildMixedFilenameInDir(const std::string& dir, unsigned int sampleRate, unsigned int channels) {
    std::ostringstream oss;
    oss << dir << "/mixed_" << sampleRate
        << "Hz_" << channels << "ch.pcm";
    return oss.str();
}

AudioRawHandler::AudioRawHandler() {
    outDir_ = "recordings/" + timestampForFile();
    ensureDir("recordings");
    ensureDir(outDir_);
    
    // Initialize streaming system
    streamer_ = std::make_unique<AudioStreamer>();
}

AudioRawHandler::~AudioRawHandler() {
    disableStreaming();
    unsubscribe();
}

bool AudioRawHandler::requestRecordingPermission() {
    if (!meetingService_) {
        std::cerr << "Cannot request recording permission: no meeting service" << std::endl;
        return false;
    }
    
    auto* recordingController = meetingService_->GetMeetingRecordingController();
    if (!recordingController) {
        std::cerr << "Recording controller not available - meeting may not support recording features" << std::endl;
        return false;
    }
    
    std::cout << "\n[RECORDING] Checking if host supports recording permission requests..." << std::endl;
    auto supportResult = recordingController->IsSupportRequestLocalRecordingPrivilege();
    if (supportResult != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS) {
        std::cerr << "[RECORDING] Host does not support recording permission requests, error: " << supportResult;
        switch(supportResult) {
            case ZOOM_SDK_NAMESPACE::SDKERR_NOT_IN_MEETING:
                std::cerr << " (NOT_IN_MEETING - Must be in meeting first)";
                break;
            case ZOOM_SDK_NAMESPACE::SDKERR_NO_PERMISSION:
                std::cerr << " (NO_PERMISSION - Bot lacks permission to request recording)";
                break;
            default:
                std::cerr << " (Meeting doesn't support participant recording requests)";
                break;
        }
        std::cerr << std::endl;
        std::cerr << "This meeting may not allow recording by participants, or recording may be automatically allowed." << std::endl;
        return false;
    }
    
    std::cout << "[RECORDING] Recording permission requests are supported. Requesting permission from host..." << std::endl;
    
    auto result = recordingController->RequestLocalRecordingPrivilege();
    
    if (result == ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS) {
        std::cout << "[RECORDING] ✓ Recording permission request sent to host successfully!" << std::endl;
        std::cout << "[RECORDING] Waiting for host approval (up to 30 seconds)..." << std::endl;
        return true;
    } else {
        std::cerr << "[RECORDING] Failed to request recording permission from host, error: " << result;
        switch(result) {
            case ZOOM_SDK_NAMESPACE::SDKERR_NO_PERMISSION:
                std::cerr << " (NO_PERMISSION - Not allowed to request recording permission)";
                break;
            case ZOOM_SDK_NAMESPACE::SDKERR_MEETING_DONT_SUPPORT_FEATURE:
                std::cerr << " (FEATURE_NOT_SUPPORTED - Meeting doesn't support recording requests)";
                break;
            case ZOOM_SDK_NAMESPACE::SDKERR_NOT_IN_MEETING:
                std::cerr << " (NOT_IN_MEETING - Must be in meeting first)";
                break;
            case ZOOM_SDK_NAMESPACE::SDKERR_WRONG_USAGE:
                std::cerr << " (WRONG_USAGE - Permission already requested or invalid state)";
                break;
            default:
                std::cerr << " (Unknown error code)";
                break;
        }
        std::cerr << std::endl;
        return false;
    }
}

bool AudioRawHandler::startRecording() {
    if (!meetingService_) {
        std::cerr << "Cannot start recording: no meeting service" << std::endl;
        return false;
    }
    
    auto* recordingController = meetingService_->GetMeetingRecordingController();
    if (!recordingController) {
        std::cerr << "Recording controller not available" << std::endl;
        return false;
    }
    
    std::cout << "\n[RECORDING] Checking if raw recording is allowed..." << std::endl;
    auto canStartResult = recordingController->CanStartRawRecording();
    if (canStartResult != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS) {
        std::cerr << "[RECORDING] Cannot start raw recording, error: " << canStartResult << std::endl;
        return false;
    }
    
    std::cout << "[RECORDING] Starting raw recording..." << std::endl;
    auto result = recordingController->StartRawRecording();
    
    if (result == ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS) {
        std::cout << "[RECORDING] ✓ Raw recording started successfully!" << std::endl;
        return true;
    } else {
        std::cerr << "[RECORDING] Failed to start raw recording, error: " << result;
        switch(result) {
            case ZOOM_SDK_NAMESPACE::SDKERR_NO_PERMISSION:
                std::cerr << " (NO_PERMISSION - Not allowed to start raw recording)";
                break;
            case ZOOM_SDK_NAMESPACE::SDKERR_WRONG_USAGE:
                std::cerr << " (WRONG_USAGE - Raw recording already in progress or invalid state)";
                break;
            default:
                std::cerr << " (Unknown error code)";
                break;
        }
        std::cerr << std::endl;
        return false;
    }
}

bool AudioRawHandler::stopRecording() {
    if (!meetingService_) {
        std::cerr << "Cannot stop recording: no meeting service" << std::endl;
        return false;
    }
    
    auto* recordingController = meetingService_->GetMeetingRecordingController();
    if (!recordingController) {
        std::cerr << "Recording controller not available" << std::endl;
        return false;
    }
    
    std::cout << "\n[RECORDING] Stopping raw recording..." << std::endl;
    auto result = recordingController->StopRawRecording();
    
    if (result == ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS) {
        std::cout << "[RECORDING] ✓ Raw recording stopped successfully!" << std::endl;
        
        // Convert all PCM files to WAV format for easy playback
        convertAllPCMToWAV();
        
        return true;
    } else {
        std::cerr << "[RECORDING] Failed to stop raw recording, error: " << result;
        switch(result) {
            case ZOOM_SDK_NAMESPACE::SDKERR_WRONG_USAGE:
                std::cerr << " (WRONG_USAGE - No recording in progress)";
                break;
            default:
                std::cerr << " (Unknown error code)";
                break;
        }
        std::cerr << std::endl;
        return false;
    }
}

bool AudioRawHandler::subscribe(bool withInterpreters) {
    auto* helper = ZOOM_SDK_NAMESPACE::GetAudioRawdataHelper();
    if (!helper) {
        std::cerr << "Audio raw data helper not available (not in meeting or helper unavailable)." << std::endl;
        return false;
    }
    
    std::cout << "[AUDIO] Attempting to subscribe to raw audio data..." << std::endl;
    std::cout << "[AUDIO] Using withInterpreters = " << (withInterpreters ? "true" : "false") << std::endl;
    
    auto err = helper->subscribe(this, withInterpreters);
    std::cout << "[AUDIO] Subscribe result: " << err << std::endl;
    
    if (err != ZOOM_SDK_NAMESPACE::SDKERR_SUCCESS) {
        std::cerr << "Failed to subscribe to audio raw data, error: " << err;
        switch (err) {
            case ZOOM_SDK_NAMESPACE::SDKERR_NO_PERMISSION:
                std::cerr << " (NO_PERMISSION)" << std::endl;
                std::cerr << "This typically means raw data access is not enabled for this Meeting SDK app." << std::endl;
                std::cerr << "Please check that Raw Data is enabled in the Zoom App Marketplace for your app." << std::endl;
                break;
            case ZOOM_SDK_NAMESPACE::SDKERR_NOT_IN_MEETING:
                std::cerr << " (NOT_IN_MEETING)" << std::endl;
                break;
            case ZOOM_SDK_NAMESPACE::SDKERR_UNINITIALIZE:
                std::cerr << " (UNINITIALIZE)" << std::endl;
                break;
            case ZOOM_SDK_NAMESPACE::SDKERR_WRONG_USAGE:
                std::cerr << " (WRONG_USAGE)" << std::endl;
                break;
            default:
                std::cerr << " (UNKNOWN ERROR)" << std::endl;
                break;
        }
        return false;
    }
    std::cout << "[AUDIO] ✓ Subscribed to audio raw data callbacks successfully!" << std::endl;
    return true;
}

void AudioRawHandler::unsubscribe() {
    auto* helper = ZOOM_SDK_NAMESPACE::GetAudioRawdataHelper();
    if (helper) {
        helper->unSubscribe();
    }
    
    // Stop raw archiving permission
    if (meetingService_) {
        auto* controller = meetingService_->GetMeetingRawArchivingController();
        if (controller) {
            controller->StopRawArchiving();
            std::cout << "Stopped raw archiving" << std::endl;
        }
    }
    
    std::lock_guard<std::mutex> lk(mtx_);
    mixedFile_.reset();
    userFiles_.clear();
}

bool AudioRawHandler::enableStreaming(const std::string& backend_type, const std::string& config) {
    if (!streamer_) {
        streamer_ = std::make_unique<AudioStreamer>();
    }
    
    if (!streamer_->initialize(backend_type, config)) {
        std::cerr << "[AUDIO] Failed to initialize audio streaming" << std::endl;
        return false;
    }
    
    streamer_->start();
    std::cout << "[AUDIO] ✓ Audio streaming enabled (" << backend_type << " -> " << config << ")" << std::endl;
    return true;
}

void AudioRawHandler::disableStreaming() {
    if (streamer_) {
        streamer_->stop();
        std::cout << "[AUDIO] Audio streaming disabled" << std::endl;
    }
}

void AudioRawHandler::writeToFile(PCMFile& file, AudioRawData* data_) {
    if (!data_) return;
    if (!data_->CanAddRef()) return; // ensure buffer validity beyond callback if needed
    data_->AddRef();
    file.write(data_->GetBuffer(), data_->GetBufferLen());
    file.flush();
    data_->Release();
}

void AudioRawHandler::streamAudioData(uint32_t user_id, const std::string& user_name, AudioRawData* data_) {
    if (!data_ || !streamer_ || !streamer_->isConnected()) return;
    
    // Stream the audio data to our processing service
    streamer_->queueAudio(
        user_id, 
        user_name,
        data_->GetBuffer(), 
        data_->GetBufferLen(),
        data_->GetSampleRate(), 
        data_->GetChannelNum()
    );
}

void AudioRawHandler::onMixedAudioRawDataReceived(AudioRawData* data_) {
    if (!data_) return;
    std::lock_guard<std::mutex> lk(mtx_);
    if (!mixedFile_) {
        auto path = buildMixedFilenameInDir(outDir_, data_->GetSampleRate(), data_->GetChannelNum());
        mixedFile_ = std::make_unique<PCMFile>(path);
        if (!mixedFile_->good()) {
            std::cerr << "Failed to open mixed PCM file for writing" << std::endl;
            mixedFile_.reset();
            return;
        }
        std::cout << "Writing mixed audio to " << path << std::endl;
    }
    writeToFile(*mixedFile_, data_);
    
    // Stream mixed audio (using special user_id 0 for mixed audio)
    streamAudioData(0, "Mixed_Audio", data_);
}

void AudioRawHandler::onOneWayAudioRawDataReceived(AudioRawData* data_, uint32_t user_id) {
    if (!data_) return;
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = userFiles_.find(user_id);
    if (it == userFiles_.end()) {
        std::string name = displayNameForUser(user_id);
        std::ostringstream fname;
        fname << outDir_ << "/user_" << user_id;
        if (!name.empty()) {
            fname << "_" << sanitize(name);
        }
        fname << "_" << data_->GetSampleRate() << "Hz_" << data_->GetChannelNum() << "ch.pcm";
        auto path = fname.str();
        auto file = std::make_unique<PCMFile>(path);
        if (!file->good()) {
            std::cerr << "Failed to open PCM file for user " << user_id << std::endl;
            return;
        }
        std::cout << "Writing user " << user_id << " audio to " << path << std::endl;
        it = userFiles_.emplace(user_id, std::move(file)).first;
    }
    writeToFile(*it->second, data_);
    
    // Stream individual participant audio
    std::string user_name = displayNameForUser(user_id);
    if (user_name.empty()) {
        user_name = "User_" + std::to_string(user_id);
    }
    streamAudioData(user_id, user_name, data_);
}

void AudioRawHandler::onShareAudioRawDataReceived(AudioRawData* data_, uint32_t user_id) {
    if (!data_) return;
    // Optional: record share audio as separate file
    std::lock_guard<std::mutex> lk(mtx_);
    uint32_t share_key = (user_id << 1) ^ 0xAAAAAAAA; // derive a distinct key
    auto it = userFiles_.find(share_key);
    if (it == userFiles_.end()) {
        auto path = outDir_ + "/share_user_" + std::to_string(user_id) + 
                    "_" + std::to_string(data_->GetSampleRate()) + "Hz_" + 
                    std::to_string(data_->GetChannelNum()) + "ch.pcm";
        auto file = std::make_unique<PCMFile>(path);
        if (!file->good()) {
            std::cerr << "Failed to open share PCM file for user " << user_id << std::endl;
            return;
        }
        std::cout << "Writing share audio for user " << user_id << " to " << path << std::endl;
        it = userFiles_.emplace(share_key, std::move(file)).first;
    }
    writeToFile(*it->second, data_);
}

void AudioRawHandler::onOneWayInterpreterAudioRawDataReceived(AudioRawData* data_, const zchar_t* pLanguageName) {
    if (!data_) return;
    std::string lang = pLanguageName ? sanitize(pLanguageName) : std::string("unknown");
    std::lock_guard<std::mutex> lk(mtx_);
    // Use a fixed key space above UINT32_MAX for interpreters by using path map only
    auto path = outDir_ + "/interpreter_" + lang + "_" + std::to_string(data_->GetSampleRate()) + "Hz_" + std::to_string(data_->GetChannelNum()) + "ch.pcm";
    // Create/open per language file lazily each callback
    PCMFile file(path);
    if (!file.good()) {
        std::cerr << "Failed to open interpreter PCM file for language " << lang << std::endl;
        return;
    }
    writeToFile(file, data_);
}

std::string AudioRawHandler::displayNameForUser(uint32_t user_id) {
    if (!meetingService_) return {};
    auto* pc = meetingService_->GetMeetingParticipantsController();
    if (!pc) return {};
    auto* info = pc->GetUserByUserID(user_id);
    if (!info) return {};
    const zchar_t* name = info->GetUserName();
    if (!name) return {};
    return std::string(name);
}

// WAV file header structure
struct WAVHeader {
    // RIFF header
    char riff_header[4] = {'R', 'I', 'F', 'F'};
    uint32_t wav_size;
    char wave_header[4] = {'W', 'A', 'V', 'E'};
    
    // fmt subchunk
    char fmt_header[4] = {'f', 'm', 't', ' '};
    uint32_t fmt_chunk_size = 16;
    uint16_t audio_format = 1; // PCM
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t sample_alignment;
    uint16_t bit_depth;
    
    // data subchunk
    char data_header[4] = {'d', 'a', 't', 'a'};
    uint32_t data_bytes;
};

bool AudioRawHandler::convertPCMToWAV(const std::string& pcmFilePath, const std::string& wavFilePath, 
                                      uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample) {
    std::ifstream pcmFile(pcmFilePath, std::ios::binary);
    if (!pcmFile) {
        std::cerr << "Failed to open PCM file: " << pcmFilePath << std::endl;
        return false;
    }
    
    // Get PCM file size
    pcmFile.seekg(0, std::ios::end);
    uint32_t pcmDataSize = static_cast<uint32_t>(pcmFile.tellg());
    pcmFile.seekg(0, std::ios::beg);
    
    if (pcmDataSize == 0) {
        std::cerr << "PCM file is empty: " << pcmFilePath << std::endl;
        return false;
    }
    
    // Create WAV header
    WAVHeader header;
    header.num_channels = channels;
    header.sample_rate = sampleRate;
    header.bit_depth = bitsPerSample;
    header.byte_rate = sampleRate * channels * (bitsPerSample / 8);
    header.sample_alignment = channels * (bitsPerSample / 8);
    header.data_bytes = pcmDataSize;
    header.wav_size = sizeof(WAVHeader) - 8 + pcmDataSize; // Total file size - 8 bytes (RIFF header)
    
    // Create WAV file
    std::ofstream wavFile(wavFilePath, std::ios::binary);
    if (!wavFile) {
        std::cerr << "Failed to create WAV file: " << wavFilePath << std::endl;
        return false;
    }
    
    // Write WAV header
    wavFile.write(reinterpret_cast<const char*>(&header), sizeof(header));
    
    // Copy PCM data
    char buffer[8192];
    while (pcmFile.read(buffer, sizeof(buffer)) || pcmFile.gcount() > 0) {
        wavFile.write(buffer, pcmFile.gcount());
    }
    
    std::cout << "Converted " << pcmFilePath << " to " << wavFilePath 
              << " (" << sampleRate << " Hz, " << channels << " channels, " 
              << bitsPerSample << " bits)" << std::endl;
    
    return true;
}

void AudioRawHandler::convertAllPCMToWAV() const {
    DIR* dir = opendir(outDir_.c_str());
    if (!dir) {
        std::cerr << "Output directory does not exist: " << outDir_ << std::endl;
        return;
    }
    
    std::cout << "\n[WAV CONVERSION] Converting all PCM files to WAV format..." << std::endl;
    int converted = 0;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename(entry->d_name);
        
        // Check if it's a .pcm file
        if (filename.size() > 4 && filename.substr(filename.size() - 4) == ".pcm") {
            std::string pcmPath = outDir_ + "/" + filename;
            
            // Check if it's a regular file
            struct stat fileStat;
            if (stat(pcmPath.c_str(), &fileStat) == 0 && S_ISREG(fileStat.st_mode)) {
                // Remove .pcm extension for base name
                std::string baseName = filename.substr(0, filename.size() - 4);
                
                // Extract sample rate and channels from filename
                // Expected format: mixed_48000Hz_2ch.pcm or user_12345_DisplayName_48000Hz_1ch.pcm
                uint32_t sampleRate = 48000; // default
                uint16_t channels = 2;       // default
                
                size_t hzPos = baseName.find("Hz_");
                size_t chPos = baseName.find("ch");
                
                if (hzPos != std::string::npos && chPos != std::string::npos) {
                    // Find start of sample rate (work backwards from Hz)
                    size_t rateStart = hzPos;
                    while (rateStart > 0 && std::isdigit(baseName[rateStart - 1])) {
                        rateStart--;
                    }
                    
                    if (rateStart < hzPos) {
                        try {
                            sampleRate = std::stoul(baseName.substr(rateStart, hzPos - rateStart));
                        } catch (const std::exception&) {
                            // Keep default on parse error
                        }
                    }
                    
                    // Extract channels (should be right after Hz_)
                    size_t chStart = hzPos + 3; // Skip "Hz_"
                    if (chStart < chPos) {
                        try {
                            channels = std::stoul(baseName.substr(chStart, chPos - chStart));
                        } catch (const std::exception&) {
                            // Keep default on parse error
                        }
                    }
                }
                
                // Create WAV filename
                std::string wavPath = outDir_ + "/" + baseName + ".wav";
                
                // Convert to WAV
                if (convertPCMToWAV(pcmPath, wavPath, sampleRate, channels, 16)) {
                    converted++;
                }
            }
        }
    }
    
    closedir(dir);
    
    std::cout << "[WAV CONVERSION] Converted " << converted << " PCM files to WAV format." << std::endl;
    std::cout << "[WAV CONVERSION] WAV files can now be played with any standard audio player." << std::endl;
}

} // namespace ZoomBot
