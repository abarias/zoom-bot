cmake_minimum_required(VERSION 3.10)
project(zoom_poc)

set(CMAKE_CXX_STANDARD 14)

# Find libcurl package
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)

include_directories(${CURL_INCLUDE_DIR})
include_directories(${GLIB_INCLUDE_DIRS})

# Include headers
include_directories(/usr/local/zoom-sdk/h)
include_directories(/usr/local/zoom-sdk/h/rawdata)

# Executable
add_executable(zoom_poc 
    src/main.cpp
    src/zoom_auth.cpp
    src/jwt_helper.cpp
    src/meeting_event_handler.cpp
    src/auth_event_handler.cpp
    src/meeting_detector.cpp
    src/sdk_initializer.cpp
    src/audio_raw_handler.cpp)

# Test executable
add_executable(test_auth
    src/test_auth.cpp
    src/jwt_helper.cpp
    src/zoom_auth.cpp)

# WAV converter utility
add_executable(wav_converter
    src/wav_converter.cpp
    src/audio_raw_handler.cpp)

# Link SDK libs
target_link_libraries(zoom_poc
    /usr/local/zoom-sdk/libmeetingsdk.so
    /usr/local/zoom-sdk/libcml.so
    /usr/local/zoom-sdk/libmpg123.so

    # System libs
    X11
    GL
    xcb
    xcb-shm
    xcb-randr
    xcb-xfixes
    xcb-shape
    xcb-keysyms
    xcb-image
    xcb-xtest
    dbus-1
    drm
    gbm
    ${CURL_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${GLIB_LIBRARIES}
)

# Link test executable
target_link_libraries(test_auth
    /usr/local/zoom-sdk/libmeetingsdk.so
    /usr/local/zoom-sdk/libcml.so
    /usr/local/zoom-sdk/libmpg123.so

    # System libs
    X11
    GL
    xcb
    xcb-shm
    xcb-randr
    xcb-xfixes
    xcb-shape
    xcb-keysyms
    xcb-image
    xcb-xtest
    dbus-1
    drm
    gbm
    ${CURL_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${GLIB_LIBRARIES}
)

# Link WAV converter utility (minimal dependencies)
target_link_libraries(wav_converter
    /usr/local/zoom-sdk/libmeetingsdk.so
    /usr/local/zoom-sdk/libcml.so
    /usr/local/zoom-sdk/libmpg123.so

    # System libs
    X11
    GL
    xcb
    xcb-shm
    xcb-randr
    xcb-xfixes
    xcb-shape
    xcb-keysyms
    xcb-image
    xcb-xtest
    dbus-1
    drm
    gbm
    ${CURL_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${GLIB_LIBRARIES}
)
