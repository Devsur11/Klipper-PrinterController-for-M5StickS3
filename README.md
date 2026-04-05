# Klipper PrinterController for M5StickS3

A complete Klipper printer controller application running on the M5StickS3 ESP32-S3 device. Control your 3D printer remotely with WiFi connectivity, intuitive UI, and comprehensive printer management features.

## Features

### Core Functionality
- **WiFi Setup & Network Configuration** - Connect to your WiFi and configure printer IP
- **Print Management** - Start, pause, resume, and cancel prints from SD card files
- **Real-time Print Status** - Monitor print progress, time remaining, and printer statistics
- **Temperature Control** - Control bed and nozzle heating with live temperature monitoring
- **Axis Movement** - Manual control of X, Y, Z, and E axes via integrated MMU
- **Power Device Control** - Toggle Moonraker power devices (relays, smart switches, etc.)
- **Emergency Stop (E-Stop)** - Immediate emergency stop capability
- **Printer Information** - View WiFi status, IP addresses, and connectivity
- **Settings Management** - Configure WiFi, printer IP, brightness, and factory reset

### User Interface
- Clean, intuitive menu-driven interface optimized for 1.14" display (135x240)
- Color-coded status indicators
- Real-time temperature graphs and heating diagrams
- Progress bars for print visualization
- Navigation via two physical buttons (A and B)
- Header and footer with context-aware controls

## Hardware Requirements

### M5StickS3 Specifications
- **Processor**: ESP32-S3 dual-core @ 240MHz
- **Memory**: 8MB Flash + 8MB PSRAM
- **Display**: 1.14" ST7789P3 LCD (135x240 resolution)
- **Connectivity**: 2.4 GHz WiFi
- **Power**: 250mAh battery, USB-C charging
- **Sensors**: 6-axis IMU (BMI270)
- **Audio**: ES8311 codec with microphone and speaker
- **Buttons**: 2 programmable buttons + side button
- **Expansion**: Hat2-Bus, HY2.0-4P port

### Software Requirements
- Klipper printer running Moonraker API server
- Moonraker version with WebSocket/JSON-RPC support
- WiFi access point near the printer setup

## Installation & Setup

### 1. Prerequisites
```bash
# Install PlatformIO CLI
pip install platformio

# Clone or navigate to the project directory
cd klipper-controller
```

### 2. Build the Project
```bash
platformio run -e m5sticks3
```

### 3. Upload to M5StickS3
```bash
# Connect M5StickS3 via USB-C
platformio run -e m5sticks3 -t upload

# Monitor serial output
platformio device monitor
```

### 4. Initial Configuration
1. Device boots to Splash Screen
2. Proceeds to WiFi Setup if no credentials stored
3. Select WiFi network and enter password
4. Enter Klipper printer IP address (default port: 7125)
5. Connects to Moonraker API and displays main menu

## Usage Guide

### Screen Navigation
- **Button A**: Scroll/Select (cycle through options)
- **Button B**: Confirm/Enter (select menu item or confirm action)
- **Side Button**: Power on/off (long press for shutdown)

### Main Menu Options

#### Print Status
- View current print job details
- Real-time progress bar and temperature display
- Time elapsed and estimated time remaining
- Pause/Resume/Cancel controls

#### File Browser
- Browse print files on printer's virtual SD card
- Select and start new print jobs
- Confirm before starting print

#### Heating Control
- Independently control bed and nozzle temperatures
- Real-time temperature monitoring
- Presets for common materials (PLA, PETG, ABS, etc.)
- Visual temperature graphs (upgrade feature)

#### Axis Control
- Manual axis movement (X, Y, Z, E)
- Configurable movement distances
- Feed rate adjustment
- Home axis controls (upgrade feature)

#### Power Control
- Toggle Moonraker power devices
- Control relay switches, smart plugs, etc.
- Device status indication (ON/OFF)
- Safety confirmations

#### Printer Info
- WiFi connection status and IP address
- Printer IP and connection status
- API connectivity status
- System information

#### Settings
- WiFi configuration and credentials management
- Printer IP setup
- Display brightness adjustment
- Factory reset option
- Version information

## Technical Architecture

### File Structure
```
src/
├── main.cpp              # Application entry point
├── config.cpp            # Configuration management
├── api_client.cpp        # Moonraker API client
├── network.cpp           # WiFi network management
├── screen_manager.cpp    # Screen switching logic
├── screens.cpp           # Screen implementations
└── display.cpp           # UI components and utilities

include/
├── config.h              # Configuration definitions
├── api_client.h          # API client interface
├── network.h             # Network manager interface
├── screen_manager.h      # Screen manager definitions
├── screens.h             # Screen class definitions
└── display.h             # Display utilities and themes
```

### Key Components

#### APIClient (api_client.cpp)
Handles all communication with Moonraker JSON-RPC API:
- Printer state queries (temperature, position, status)
- Print commands (start, pause, resume, cancel, emergency stop)
- File system queries and operations
- Power device control
- Gcode execution

#### NetworkManager (network.cpp)
Manages WiFi connectivity:
- WiFi network scanning
- Connection establishment and management
- Signal strength monitoring
- MAC address retrieval

#### ScreenManager (screen_manager.cpp)
Coordinates all UI screens:
- Screen creation and lifecycle
- Button event delegation
- Screen switching logic
- Display update coordination

#### Config (config.cpp)
Persistent configuration storage using SPIFFS:
- WiFi credentials
- Printer IP and port
- User preferences
- Machine identification

### Theme & Visual Design
- **Colors**: Blue primary, Red accents, Green success, Orange warnings
- **Typography**: Clean, readable fonts optimized for small display
- **Layout**: Efficient use of 135x240 pixel display
- **Feedback**: Status colors, progress indicators, loading spinners

## API Integration

### Moonraker JSON-RPC Methods Used

#### Machine Control
- `printer.emergency_stop` - Emergency stop the printer
- `printer.print.start` - Start a new print
- `printer.print.pause` - Pause current print
- `printer.print.resume` - Resume paused print
- `printer.print.cancel` - Cancel current print

#### Temperature Control
- `printer.heater_bed.set_temperature` - Set bed temperature
- `printer.extruder.set_temperature` - Set nozzle temperature

#### Axis Movement
- `printer.gcode.script` - Execute G-code commands

#### Power Management
- `machine.power.on` - Turn power device on
- `machine.power.off` - Turn power device off
- `machine.power.devices` - List available power devices

#### File Management
- `server.files.list` - List print files

#### Status Queries
- `printer.objects.query` - Query printer state and temperatures

## Configuration Files

### config.json (SPIFFS Storage)
```json
{
  "ssid": "Your WiFi SSID",
  "password": "WiFi Password",
  "klipperIP": "192.168.1.100",
  "klipperPort": 7125,
  "machineName": "Klipper Controller"
}
```

## Limitations & Considerations

### Display Constraints
- Small 135x240 pixel display requires careful UI design
- Menu items must be scrollable for all options
- Limited space for detailed information display
- Font readability critical for usability

### Hardware Constraints
- 250mAh battery provides ~30 minutes of continuous use
- WiFi range depends on router placement
- USB-C power recommended for extended use
- Temperature monitoring via Moonraker API only

### Network Requirements
- Requires stable WiFi connection
- Moonraker must be accessible on local network
- JSON-RPC API must be enabled on Moonraker
- Typical response time: 100-500ms over WiFi

## Future Enhancements

### Planned Features
- **Temperature Graphs** - Real-time temperature visualization
- **G-code Preview** - Display first layer preview
- **Camera Integration** - Live printer camera feed
- **Bed Leveling Assistant** - Interactive probe assist
- **Macro Execution** - Custom Klipper macro execution
- **Filament Management** - Filament type and weight tracking
- **Print History** - Statistics and past print logs
- **Voice Control** - Voice commands via built-in microphone
- **OTA Updates** - Over-the-air firmware updates
- **Moonraker Extensions** - Integration with community plugins

### UI Improvements
- Touchscreen support (with compatible hardware)
- Gesture controls
- Custom themes and color schemes
- Widget customization
- Landscape orientation support

### Performance Optimizations
- WebSocket persistent connection
- Intelligent polling and caching
- Compressed data transmission
- Memory optimization for larger displays

## Troubleshooting

### WiFi Connection Issues
1. Verify SSID and password are correct
2. Ensure router is powered and broadcasting
3. Move M5StickS3 closer to router
4. Check router WiFi channel (2.4 GHz required)
5. Factory reset and reconfigure if persistent issues

### Moonraker Connection Failed
1. Verify printer IP address is correct
2. Ensure Moonraker is running (`moonraker.service status`)
3. Check Moonraker logs: `journalctl -u moonraker`
4. Ensure firewall allows port 7125
5. Test connectivity: `ping <printer_ip>`

### Display Issues
1. Ensure M5StickS3 is powered (charging light)
2. Restart device (long hold side button)
3. Check display brightness settings in settings menu
4. Verify M5Unified library is correctly installed

### Print Failures
1. Verify printer is ready (homed, heated)
2. Check file format (must be compatible with Klipper)
3. Monitor temperature during print
4. Watch for layer shifts or adhesion issues

## Development Guidelines

### Adding New Screens
1. Create new class inheriting from `Screen` in `screens.h`
2. Implement required virtual methods:
   - `init()` - Initialize screen state
   - `draw()` - Render screen content
   - `update()` - Update state (called every loop)
   - `handleButtonA()` - Button A (increment/select)
   - `handleButtonB()` - Button B (confirm/enter)
3. Add screen type to `ScreenType` enum in `screen_manager.h`
4. Register screen in `ScreenManager::createScreen()`
5. Add navigation from existing screens

### Adding API Calls
1. Define method in `APIClient` class
2. Construct JSON-RPC request using ArduinoJson
3. Call `sendJsonRpc()` with method name and params
4. Handle response in `receiveJsonRpc()`
5. Update `PrinterState` or device list as needed

### Theme Customization
Edit color constants in `display.h`:
```cpp
namespace Theme {
    static constexpr uint16_t PRIMARY = 0x1E90FF;      // Your color
    // ... more colors
}
```

## License & Attribution

This project is designed for use with:
- **Klipper** - Open-source 3D printer firmware
- **Moonraker** - Klipper API and Web Server
- **M5Stack** - M5StickS3 hardware and libraries

## Support & Community

For issues and feature requests:
- Check existing GitHub issues
- Review Klipper documentation: https://www.klipper3d.org/
- Check Moonraker documentation: https://moonraker.readthedocs.io/
- Review M5Stack documentation: https://docs.m5stack.com/

## Contributing

Contributions welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Test thoroughly on actual hardware
4. Submit pull request with detailed description
5. Include any new dependencies in platformio.ini

## Version History

### v1.0 (Current)
- Initial release
- Core printer control functionality
- WiFi setup and configuration
- Temperature control and monitoring
- Print file management
- Power device control
- Settings menu

## Contact & Questions

For questions or suggestions, please open an issue on GitHub or contact the development team.