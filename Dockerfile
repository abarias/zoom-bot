FROM ubuntu:22.04

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install build tools & dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    wget \
    unzip \
    pkg-config \
    libssl-dev \
    python3 \
    python3-pip \
    # Core system libraries for signal handling, filesystem, and POSIX compliance  
    libc6-dev \
    # Threading support for C++ std::thread, std::atomic, std::mutex
    libpthread-stubs0-dev \
    # Zoom SDK dependencies - X11 and graphics
    libglib2.0-dev \
    libgl1-mesa-dev \
    libgl-dev \
    libx11-dev \
    libxext-dev \
    libxfixes-dev \
    # XCB (X C Binding) libraries for window management
    libxcb1-dev \
    libxcb-shm0-dev \
    libxcb-randr0-dev \
    libxcb-xfixes0-dev \
    libxcb-shape0-dev \
    libxcb-keysyms1-dev \
    libxcb-image0-dev \
    libxcb-xtest0-dev \
    # Additional XCB libraries that may be needed
    libxcb-keysyms1 \
    libxcb-image0 \
    libxcb-randr0 \
    # System integration libraries
    libdbus-1-dev \
    libdrm-dev \
    libgbm-dev \
    # Network and crypto libraries
    libcurl4-openssl-dev \
    # JSON parsing for modern C++
    nlohmann-json3-dev \
    # Qt libraries (legacy support - may not be needed in newer versions)
    qtbase5-dev \
    qtchooser \
    qt5-qmake \
    qtbase5-dev-tools \
    libqt5gui5 \
    # Audio utilities for testing (optional)
    alsa-utils \
    # PulseAudio support for audio streaming
    pulseaudio \
    pulseaudio-utils \
    libpulse-dev \
    libpulse0 \
    # ALSA PulseAudio plugin for compatibility
    libasound2-plugins \
    # File type detection utility
    file \
    && rm -rf /var/lib/apt/lists/*

# Install Python deps (for Deepgram integration)
COPY requirements.txt /app/requirements.txt
RUN pip3 install -r /app/requirements.txt || true

WORKDIR /app

# Copy Zoom SDK into the image
COPY zoom-sdk /usr/local/zoom-sdk

# Env paths for headers + libs
ENV CPLUS_INCLUDE_PATH="/usr/local/zoom-sdk/h:$CPLUS_INCLUDE_PATH"
ENV LIBRARY_PATH="/usr/local/zoom-sdk:$LIBRARY_PATH"
ENV LD_LIBRARY_PATH="/usr/local/zoom-sdk:/usr/local/zoom-sdk/qt_libs/Qt/lib:$LD_LIBRARY_PATH"

# Audio environment variables for container
ENV PULSE_RUNTIME_PATH="/tmp/pulse-runtime"
ENV ALSA_PCM_CARD="pulse"
ENV ALSA_PCM_DEVICE="0"

# Fix missing versioned .so if only the unversioned one exists
RUN cd /usr/local/zoom-sdk && \
    if [ -f libmeetingsdk.so ] && [ ! -f libmeetingsdk.so.1 ]; then \
        ln -s libmeetingsdk.so libmeetingsdk.so.1; \
    fi

# Copy source code
COPY src/ /app/src/
COPY CMakeLists.txt /app/
COPY *.sh /app/
COPY *.md /app/
COPY *.pa /app/
COPY asound.conf /app/

# Build the application
RUN cd /app && mkdir -p build && cd build && cmake .. && make

# Create recordings directory
RUN mkdir -p /app/recordings

# Set executable permissions for scripts
RUN chmod +x /app/*.sh

# Create PulseAudio configuration directory and copy virtual audio config
RUN mkdir -p /etc/pulse && \
    cp /app/pulseaudio-virtual.pa /etc/pulse/virtual-audio.pa && \
    cp /app/asound.conf /etc/asound.conf

# Set up entrypoint for virtual audio initialization
COPY docker-entrypoint.sh /usr/local/bin/entrypoint.sh
COPY run-zoom-bot.sh /usr/local/bin/run-zoom-bot.sh
RUN chmod +x /usr/local/bin/entrypoint.sh /usr/local/bin/run-zoom-bot.sh

# Set entrypoint
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]

# Default command
CMD ["bash"]