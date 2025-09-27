#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

std::atomic<bool> exitRequested{false};

void signalHandler(int signal) {
    std::cout << "\n[TEST] Received signal " << signal << " - setting exit flag" << std::endl;
    exitRequested.store(true);
}

int main() {
    // Set up signal handling
    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        std::cerr << "Failed to set up SIGINT handler" << std::endl;
        return -1;
    }
    
    std::cout << "Signal test program started. Press Ctrl+C to test signal handling..." << std::endl;
    std::cout << "The program should exit gracefully when you press Ctrl+C." << std::endl;
    
    int counter = 0;
    while (!exitRequested.load()) {
        std::cout << "Running... " << counter++ << " (exitRequested = " << exitRequested.load() << ")" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        if (counter > 30) {
            std::cout << "Auto-exiting after 30 seconds" << std::endl;
            break;
        }
    }
    
    if (exitRequested.load()) {
        std::cout << "[TEST] ✓ Signal handling works correctly!" << std::endl;
    } else {
        std::cout << "[TEST] Signal handling not tested (auto-exit)" << std::endl;
    }
    
    return 0;
}