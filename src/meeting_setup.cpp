#include "meeting_setup.h"
#include <iostream>
#include <algorithm>
#include <cctype>

namespace ZoomBot {

MeetingSetup::MeetingDetails MeetingSetup::getMeetingDetailsFromConsole() {
    MeetingDetails details;
    
    std::cout << "\n🎥 Zoom Bot Meeting Setup" << std::endl;
    std::cout << "=========================" << std::endl;
    
    // Get meeting number
    std::cout << "\nEnter meeting number (format: XXX XXXX XXXX): ";
    std::string meetingInput;
    std::getline(std::cin, meetingInput);
    
    details.meetingNumber = parseMeetingNumber(meetingInput);
    if (details.meetingNumber.empty()) {
        details.success = false;
        details.errorMessage = "Invalid meeting number format. Expected: XXX XXXX XXXX (11 digits)";
        std::cerr << "❌ " << details.errorMessage << std::endl;
        return details;
    }
    
    std::cout << "✅ Meeting number: " << details.meetingNumber << std::endl;
    
    // Get meeting password
    std::cout << "Enter meeting password: ";
    std::getline(std::cin, details.password);
    details.password = trim(details.password);
    
    if (details.password.empty()) {
        details.success = false;
        details.errorMessage = "Meeting password cannot be empty";
        std::cerr << "❌ " << details.errorMessage << std::endl;
        return details;
    }
    
    std::cout << "✅ Password entered" << std::endl;
    
    // Confirmation
    details.success = confirmDetails(details.meetingNumber, details.password);
    if (!details.success) {
        details.errorMessage = "Setup cancelled by user";
    }
    
    return details;
}

std::string MeetingSetup::trim(const std::string& str) {
    auto start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    auto end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

std::string MeetingSetup::parseMeetingNumber(const std::string& input) {
    std::string trimmed = trim(input);
    std::string cleaned;
    
    // Remove all spaces
    for (char c : trimmed) {
        if (c != ' ') {
            cleaned += c;
        }
    }
    
    // Validate format
    if (cleaned.length() != 11) {
        return "";
    }
    
    for (char c : cleaned) {
        if (!std::isdigit(c)) {
            return "";
        }
    }
    
    return cleaned;
}

bool MeetingSetup::confirmDetails(const std::string& meetingNumber, const std::string& password) {
    std::cout << "\n📋 Meeting Details:" << std::endl;
    std::cout << "  Number: " << meetingNumber << std::endl;
    std::cout << "  Password: " << std::string(password.length(), '*') << std::endl;
    std::cout << "\nProceed? (y/N): ";
    
    std::string confirm;
    std::getline(std::cin, confirm);
    confirm = trim(confirm);
    std::transform(confirm.begin(), confirm.end(), confirm.begin(), ::tolower);
    
    bool confirmed = (confirm == "y" || confirm == "yes");
    if (confirmed) {
        std::cout << "✅ Confirmed!" << std::endl;
    } else {
        std::cout << "❌ Cancelled." << std::endl;
    }
    
    return confirmed;
}

}