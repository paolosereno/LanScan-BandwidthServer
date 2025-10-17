cmake_minimum_required(VERSION 3.16)

project(LanScan-BandwidthServer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC specific: Enable proper C++17 __cplusplus macro
if(MSVC)
    add_compile_options(/Zc:__cplusplus /permissive-)
endif()

set(CMAKE_AUTOMOC ON)

# Find Qt6 packages (minimal dependencies)
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Network
)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Source files
set(SOURCES
    src/main.cpp
    src/BandwidthServer.cpp
    src/TcpHandler.cpp
    src/UdpHandler.cpp
    src/Protocol.cpp
    src/Statistics.cpp
)

# Header files (for MOC)
set(HEADERS
    include/BandwidthServer.h
    include/TcpHandler.h
    include/UdpHandler.h
    include/Protocol.h
    include/Statistics.h
)

# Create executable
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
)

# Link Qt libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Network
)

# Installation rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Install configuration file
install(FILES
    bandwidth-server.conf
    DESTINATION bin
    OPTIONAL
)

# Platform-specific settings
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE FALSE  # Console application
    )
endif()

if(UNIX AND NOT APPLE)
    # Linux-specific settings
    install(FILES docs/bandwidth-server.service
        DESTINATION /etc/systemd/system
        OPTIONAL
    )
endif()

# Enable testing
enable_testing()

# Add test subdirectory (future)
# add_subdirectory(tests)
