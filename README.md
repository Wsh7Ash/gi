# Gmail Infinity Factory — C++ Rewrite

A modern C++20 rewrite of the Gmail Infinity Factory, featuring high-performance account creation with advanced stealth capabilities.

## Features

- **Multi-threaded Account Creation**: Concurrent Gmail account generation with configurable thread pools
- **Advanced Browser Automation**: Chrome DevTools Protocol (CDP) control for maximum stealth
- **Proxy Management**: Rotating residential, mobile, and IPv6 proxies with health monitoring
- **Identity Generation**: Realistic persona creation with geo-targeted profiles
- **Verification Integration**: SMS, CAPTCHA, email recovery, and voice verification
- **Account Warming**: Automated activity simulation for reputation building
- **API & Dashboard**: REST API and real-time WebSocket monitoring
- **Terminal UI**: Rich interactive dashboard using FTXUI

## Technology Stack

- **Language**: C++20
- **Build System**: CMake 3.25+ with vcpkg package manager
- **Async I/O**: Boost.Asio
- **HTTP/WebSocket**: Boost.Beast, libcurl, cpp-httplib
- **Web Framework**: Crow (C++ micro-framework)
- **JSON/YAML**: nlohmann/json, yaml-cpp
- **Logging**: spdlog
- **Encryption**: OpenSSL
- **Terminal UI**: FTXUI
- **CLI**: CLI11

## Prerequisites

- Windows 10/11 with Visual Studio 2022 Build Tools or MSVC
- vcpkg package manager
- Git

## Installation

### 1. Install vcpkg

```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
```

### 2. Clone and Build

```bash
git clone https://github.com/Wsh7Ash/gi.git
cd gi
vcpkg install --triplet x64-windows
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

### 3. Configuration

Copy and configure the settings file:

```bash
cp config/settings.yaml.example config/settings.yaml
# Edit config/settings.yaml with your credentials and preferences
```

## Usage

### Basic Usage

```bash
# Run with default settings
./build/gmail_infinity

# Show help
./build/gmail_infinity --help

# Mock mode (no real browser, for testing)
./build/gmail_infinity --mock

# Custom thread count
./build/gmail_infinity --threads 8

# Custom config file
./build/gmail_infinity --config config/custom_settings.yaml
```

### Interactive Dashboard

The application features a rich terminal dashboard with real-time metrics:

- Account creation progress
- Proxy health status
- Thread activity monitoring
- Success/failure statistics
- System resource usage

### API Usage

Start the REST API and WebSocket server:

```bash
./build/gmail_infinity --api --port 8080
```

Available endpoints:
- `GET /health` - Service health check
- `POST /v1/accounts/create` - Create new account
- `GET /v1/accounts/list` - List created accounts
- `GET /v1/accounts/export` - Export accounts as CSV

WebSocket endpoint: `ws://localhost:8080/ws`

## Configuration

### Main Settings (`config/settings.yaml`)

```yaml
# Account creation settings
creation:
  max_threads: 8
  delay_between_accounts: 30s
  retry_attempts: 3

# Proxy settings
proxies:
  residential_file: "config/proxies.txt"
  mobile_api_key: "your_mobile_api_key"
  rotation_strategy: "round_robin"

# Browser settings
browser:
  headless: true
  user_agent_rotation: true
  stealth_mode: true
  chrome_path: "C:/Program Files/Google/Chrome/Application/chrome.exe"

# Verification settings
verification:
  sms_provider: "5sim"
  captcha_solver: "2captcha"
  temp_email_provider: "mail.tm"

# Logging
logging:
  level: "info"
  file: "logs/gmail_infinity.log"
  max_size: "100MB"
  max_files: 10
```

### Fingerprint Database (`config/fingerprints.json`)

Contains browser fingerprints for stealth mode. Auto-generated from real browser data.

### Proxy List (`config/proxies.txt`)

Format: `ip:port:username:password:type`

## Architecture

```
src/
├── main.cpp                    # Application entry point
├── app/                        # Core application logic
│   ├── orchestrator.{h,cpp}    # Main orchestration engine
│   ├── metrics.{h,cpp}         # Performance metrics
│   ├── secure_vault.{h,cpp}    # Encrypted credential storage
│   └── app_logger.{h,cpp}      # Application logging
├── core/                       # Core engine components
│   ├── proxy_manager.{h,cpp}   # Proxy rotation & health checking
│   ├── browser_controller.{h,cpp} # CDP browser automation
│   ├── fingerprint_generator.{h,cpp} # Browser fingerprinting
│   ├── behavior_engine.{h,cpp} # Human-like interaction simulation
│   ├── detection_evasion.{h,cpp} # Anti-detection techniques
│   └── cloak_launcher.{h,cpp}  # CloakBrowser integration
├── identity/                   # Persona generation
│   ├── persona_generator.{h,cpp}
│   ├── name_generator.{h,cpp}
│   ├── bio_generator.{h,cpp}
│   └── photo_generator.{h,cpp}
├── verification/               # Verification services
│   ├── sms_providers.{h,cpp}
│   ├── captcha_solver.{h,cpp}
│   ├── email_recovery.{h,cpp}
│   └── voice_verification.{h,cpp}
├── creators/                   # Account creation flows
│   ├── gmail_creator.{h,cpp}
│   ├── web_creator.{h,cpp}
│   ├── android_creator.{h,cpp}
│   ├── workspace_creator.{h,cpp}
│   └── recovery_link_creator.{h,cpp}
├── warming/                    # Account warming & reputation
│   ├── activity_simulator.{h,cpp}
│   ├── google_services.{h,cpp}
│   └── reputation_builder.{h,cpp}
└── api/                        # API & dashboard
    ├── websocket_server.{h,cpp}
    ├── rest_server.{h,cpp}
    └── dashboard.{h,cpp}
```

## Security

- All credentials encrypted with AES-256-GCM
- Secure proxy rotation with anonymization
- Browser fingerprint randomization
- Anti-detection JavaScript injection
- Rate limiting and request throttling

## Performance

- Multi-threaded architecture with Boost.Asio thread pools
- Non-blocking I/O for all network operations
- Memory-efficient proxy pool management
- Optimized browser automation via CDP
- Real-time metrics with minimal overhead

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Disclaimer

This software is for educational purposes only. Users are responsible for complying with all applicable laws and terms of service.

## Support

For issues and questions:
- Open an issue on GitHub
- Check the [Wiki](https://github.com/Wsh7Ash/gi/wiki) for documentation
- Join our Discord community (link in README)
