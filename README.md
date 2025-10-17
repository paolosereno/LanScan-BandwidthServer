# LanScan Bandwidth Test Server

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![Qt](https://img.shields.io/badge/Qt-6.x-brightgreen.svg)](https://www.qt.io/)
[![CMake](https://img.shields.io/badge/CMake-3.16+-064F8C.svg)](https://cmake.org/)

Lightweight bandwidth testing server for [LanScan](https://github.com/paolosereno/LanScan) network scanner application.

## Overview

The LanScan Bandwidth Test Server is a companion application that runs on target devices to enable accurate network bandwidth measurements from the LanScan client. It provides a simple, efficient server that handles TCP and UDP bandwidth tests.

## Features

- ✅ **TCP Server** - Reliable connection-oriented bandwidth testing
- ✅ **UDP Server** - High-speed connectionless bandwidth testing
- ✅ **Download Tests** - Server sends data to client
- ✅ **Upload Tests** - Server receives data from client
- ✅ **Configurable Parameters** - Port, duration, packet size
- ✅ **Statistics Tracking** - Bytes transferred, packets, packet loss
- ✅ **Cross-Platform** - Windows, Linux, macOS support
- ✅ **Lightweight** - Minimal dependencies (Qt Core + Network only)
- ✅ **Console Application** - No GUI, runs in background

## Requirements

- Qt 6.2 or higher
- CMake 3.16 or higher
- C++17 compatible compiler
  - Windows: MSVC 2019+, MinGW GCC 9+
  - Linux: GCC 9+, Clang 10+
  - macOS: Xcode 11+

## Build Instructions

### Windows (MinGW)

```bash
# Clone repository
git clone https://github.com/paolosereno/LanScan-BandwidthServer.git
cd LanScan-BandwidthServer

# Create build directory
mkdir build && cd build

# Configure with MinGW
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="C:/Qt/6.9.1/mingw_64" ..

# Build
cmake --build . -j8

# Run
./LanScan-BandwidthServer.exe
```

### Windows (MSVC)

```bash
# Configure with MSVC
cmake -G "Visual Studio 17 2022" -A x64 \
  -DCMAKE_PREFIX_PATH="C:/Qt/6.9.1/msvc2022_64" ..

# Build Release
cmake --build . --config Release -j8

# Run
./Release/LanScan-BandwidthServer.exe
```

### Linux

```bash
# Install dependencies
sudo apt-get install qt6-base-dev cmake build-essential

# Clone and build
git clone https://github.com/paolosereno/LanScan-BandwidthServer.git
cd LanScan-BandwidthServer
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)

# Run
./LanScan-BandwidthServer
```

### macOS

```bash
# Install Qt via Homebrew
brew install qt@6 cmake

# Clone and build
git clone https://github.com/paolosereno/LanScan-BandwidthServer.git
cd LanScan-BandwidthServer
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)" ..
cmake --build . -j$(sysctl -n hw.ncpu)

# Run
./LanScan-BandwidthServer
```

## Usage

### Basic Usage

```bash
# Start server with defaults (port 5201)
./LanScan-BandwidthServer

# Output:
# LanScan Bandwidth Test Server v1.0.0
# TCP Server listening on 0.0.0.0:5201
# UDP Server listening on 0.0.0.0:5201
# Ready to accept connections...
```

### Command-Line Options

```bash
# Custom port
./LanScan-BandwidthServer --port 8080

# TCP only
./LanScan-BandwidthServer --tcp-only

# UDP only
./LanScan-BandwidthServer --udp-only

# Verbose logging
./LanScan-BandwidthServer --verbose

# Custom config file
./LanScan-BandwidthServer --config /path/to/bandwidth-server.conf

# Show version
./LanScan-BandwidthServer --version

# Show help
./LanScan-BandwidthServer --help
```

### Configuration File

Create `bandwidth-server.conf`:

```ini
[Server]
tcp_port=5201
udp_port=5201
max_connections=10
buffer_size=65536
max_test_duration=60

[Logging]
log_level=INFO
log_file=bandwidth-server.log

[Security]
enable_rate_limiting=true
max_requests_per_ip=5
request_window_seconds=60
```

### Firewall Configuration

**Linux (UFW):**
```bash
sudo ufw allow 5201/tcp
sudo ufw allow 5201/udp
```

**Windows (PowerShell as Administrator):**
```powershell
New-NetFirewallRule -DisplayName "LanScan Bandwidth Server" `
  -Direction Inbound -Protocol TCP -LocalPort 5201 -Action Allow
New-NetFirewallRule -DisplayName "LanScan Bandwidth Server" `
  -Direction Inbound -Protocol UDP -LocalPort 5201 -Action Allow
```

### Running as a Service (Linux)

Create `/etc/systemd/system/bandwidth-server.service`:

```ini
[Unit]
Description=LanScan Bandwidth Test Server
After=network.target

[Service]
Type=simple
User=lanscan
ExecStart=/usr/local/bin/LanScan-BandwidthServer
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl enable bandwidth-server
sudo systemctl start bandwidth-server
sudo systemctl status bandwidth-server
```

## Protocol

The server implements a simple text-based handshake protocol followed by binary data transfer.

### Handshake (Client → Server)
```
LANSCAN_BW_TEST
VERSION:1.0
PROTOCOL:TCP|UDP
DIRECTION:DOWNLOAD|UPLOAD
DURATION:<seconds>
PACKET_SIZE:<bytes>
END
```

### Response (Server → Client)
```
LANSCAN_BW_OK
SERVER_VERSION:1.0
READY
```

### Data Transfer
Binary packets with format: `[SEQ:8][TIME:8][DATA:N]`

### Results (Server → Client)
```
LANSCAN_BW_RESULT
BYTES_TRANSFERRED:<bytes>
DURATION:<milliseconds>
PACKETS_SENT:<count>
PACKETS_RECEIVED:<count>
PACKET_LOSS:<percentage>
END
```

For full protocol specification, see [docs/bandwidth-server-spec.md](docs/bandwidth-server-spec.md).

## Testing with LanScan Client

1. **Start the server** on the target device:
   ```bash
   ./LanScan-BandwidthServer
   ```

2. **Open LanScan** on your computer

3. **Scan the network** to find devices

4. **Double-click** a device to open Device Detail Dialog

5. **Go to Diagnostics tab** and click "Test Bandwidth"

6. **Configure test parameters**:
   - Target IP: (auto-filled)
   - Port: 5201 (default)
   - Duration: 10 seconds
   - Protocol: TCP (recommended)
   - Direction: Download or Upload
   - Packet Size: 64 KB

7. **Click OK** to start the test

8. **View results** in real-time

## Project Status

**Current Phase**: Initial Setup
**Version**: 1.0.0 (in development)

### Roadmap

- [x] Project structure and documentation
- [ ] Protocol implementation (Phase 1)
- [ ] TCP server implementation (Phase 1)
- [ ] UDP server implementation (Phase 1)
- [ ] Statistics tracking (Phase 1)
- [ ] Command-line interface (Phase 1)
- [ ] LanScan client integration (Phase 2)
- [ ] Unit tests (Phase 3)
- [ ] Integration tests (Phase 3)
- [ ] Binary distribution (Phase 4)

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## Related Projects

- [LanScan](https://github.com/paolosereno/LanScan) - Network scanner and diagnostic tool (client)

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Author

Paolo Sereno - [@paolosereno](https://github.com/paolosereno)

## Support

For issues, questions, or feature requests, please open an issue on GitHub:
https://github.com/paolosereno/LanScan-BandwidthServer/issues
