#include <iostream>
#include "../src/audio_raw_handler.h"

int main(int argc, char* argv[]) {
    std::cout << "WAV Conversion Utility for Zoom Bot Recordings\n" << std::endl;
    
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <recordings_directory>" << std::endl;
        std::cout << "Example: " << argv[0] << " ./recordings/20250924_170906" << std::endl;
        return 1;
    }
    
    std::string recordingsDir = argv[1];
    std::cout << "Converting PCM files in directory: " << recordingsDir << std::endl;
    
    // Create a temporary AudioRawHandler just for the conversion utility
    ZoomBot::AudioRawHandler handler;
    
    // Manually set the output directory for conversion
    // We'll need to make outDir_ accessible or create a static method
    
    // For now, let's test with individual file conversions
    std::cout << "Testing WAV conversion functionality..." << std::endl;
    
    // Test conversion of a few common formats
    bool success1 = ZoomBot::AudioRawHandler::convertPCMToWAV(
        recordingsDir + "/mixed_32000Hz_1ch.pcm",
        recordingsDir + "/mixed_32000Hz_1ch.wav",
        32000, 1, 16
    );
    
    bool success2 = ZoomBot::AudioRawHandler::convertPCMToWAV(
        recordingsDir + "/user_16778240_Cory_Brightman_32000Hz_1ch.pcm",
        recordingsDir + "/user_16778240_Cory_Brightman_32000Hz_1ch.wav",
        32000, 1, 16
    );
    
    bool success3 = ZoomBot::AudioRawHandler::convertPCMToWAV(
        recordingsDir + "/user_16784384_MyBot_32000Hz_1ch.pcm",
        recordingsDir + "/user_16784384_MyBot_32000Hz_1ch.wav",
        32000, 1, 16
    );
    
    int totalConverted = (success1 ? 1 : 0) + (success2 ? 1 : 0) + (success3 ? 1 : 0);
    std::cout << "\nConversion complete! Successfully converted " << totalConverted << " files." << std::endl;
    
    if (totalConverted > 0) {
        std::cout << "You can now play the WAV files with any audio player:" << std::endl;
        std::cout << "  vlc " << recordingsDir << "/*.wav" << std::endl;
        std::cout << "  mpv " << recordingsDir << "/mixed_32000Hz_1ch.wav" << std::endl;
        std::cout << "  aplay " << recordingsDir << "/mixed_32000Hz_1ch.wav" << std::endl;
    }
    
    return 0;
}