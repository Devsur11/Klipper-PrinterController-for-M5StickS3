# Klipper Controller - Quick Start Guide

## 5-Minute Setup

### Step 1: Hardware Setup
1. Connect M5StickS3 to your computer via USB-C cable
2. Ensure your Klipper printer is running and accessible on your local network
3. Note down your printer's IP address

### Step 2: Build & Upload
```bash
# From project directory
platformio run -e m5sticks3 -t upload

# Monitor the output
platformio device monitor
```

### Step 3: First Boot & Configuration
1. Device boots and shows splash screen
2. **WiFi Setup Screen** appears:
   - Press **Button A** to scroll through available networks
   - Press **Button B** to select your WiFi network
   - Device prompts for password (manual entry required)
   - Connects automatically once password is entered

3. **Printer IP Setup Screen** appears:
   - Device already has WiFi, now needs printer connection
   - Enter your Klipper printer's IP address (e.g., 192.168.1.100)
   - Default Moonraker port is 7125 (usually correct)

4. **Main Menu** appears - Setup complete!

## Main Menu Navigation

### Button Controls
- **Button A (Left)**: Scroll up/Previous option
- **Button B (Right)**: Confirm/Select current option
- **Side Button**: Long press to power off

### Quick Actions

**Start a Print**
1. Select "File Browser" from Main Menu
2. Press Button A to scroll through files
3. Press Button B to enter confirm
4. Confirm print start

**Stop/Pause Current Print**
1. Select "Print Status" from Main Menu
2. Press Button B to pause
3. Press Button A to control options

**Adjust Temperature**
1. Select "Heating Control" from Main Menu
2. Use Button A to change bed/nozzle selection
3. Use Button A to adjust temperature values
4. Press Button B to apply

**Move Axes**
1. Select "Axis Control" from Main Menu
2. Use Button A to select axis (X, Y, Z, E)
3. Use Button B to execute movement
4. Press Button A to change distance

## Common Issues & Quick Fixes

### Device Won't Connect to WiFi
- Verify SSID and password are correct
- Move closer to router
- Restart router and device
- Factory reset in Settings menu

### Can't Find Printer
- Verify printer IP address is correct
- Test: `ping <printer_ip>` from computer
- Ensure Moonraker service is running
- Check router firewall settings

### Display is Dim
- Check Settings → Brightness
- Ensure device is charging if using battery
- Device may dim when battery is low

### Prints Won't Start
- Verify file is on printer's SD card
- Check printer is homed and ready
- Ensure bed/nozzle temperatures are set
- Check printer status screen for errors

## Tips & Tricks

1. **Remote Monitoring** - Monitor prints from another room via M5StickS3
2. **Emergency Stop** - Quick E-stop available from any menu
3. **Temperature Presets** - Heating control remembers last temperatures used
4. **Portable Design** - 20g device fits pocket, magnetic back stays on printer
5. **Battery Power** - 250mAh battery works for ~30 min of active use
6. **WiFi Range** - Works up to 100ft from router (depending on obstacles)

## Screen-by-Screen Guide

### Print Status Screen
Shows current print progress with:
- Real-time temperature readings
- Progress bar and percentage
- Estimated time remaining
- Pause/Resume/Cancel buttons

### File Browser
Browse and select G-code files:
- Scrollable file list
- Confirmation before print start
- Shows file names clearly

### Heating Control
Independent temperature control:
- Bed temperature (0-120°C)
- Nozzle temperature (0-280°C)
- Real-time readouts
- Visual feedback

### Axis Control
Manual movement controls:
- X, Y, Z, E axes
- Distance per move (0.1mm to 100mm)
- Useful for leveling, filament changes, etc.

### Power Devices
Control external relays/switches via Moonraker:
- Toggle individual devices
- Status indication (ON/OFF)
- Useful for lights, fans, filters

### Printer Info
System status overview:
- WiFi connection and IP address
- Printer IP and connection status
- API connectivity indication

### Settings
Configuration and options:
- Re-configure WiFi credentials
- Change printer IP address
- Adjust display brightness
- Factory reset (clears all settings)

## Default Network Configuration

**Display**: 135x240 pixels  
**WiFi**: 2.4 GHz (802.11 b/g/n)  
**Port**: 7125 (Moonraker default)  
**Protocol**: JSON-RPC 2.0  
**Response Time**: 100-500ms typical  

## Power Management

- **USB Power**: Recommended for continuous monitoring
- **Battery Power**: ~30 minutes of active use
- **Sleep Mode**: Device enters low power when idle
- **Brightness**: Reduces automatically when on battery
- **Auto-off**: Device auto-powers off after 10 minutes idle on battery

## File Storage

Configuration saved to SPIFFS:
- WiFi credentials (encrypted at application level)
- Printer IP address
- User preferences
- Settings state

**Note**: Factory reset erases all stored data.

## Next Steps

1. **Explore Settings** - Configure your preferences
2. **Try a Test Print** - Start a simple print to verify connectivity
3. **Monitor from Distance** - Move away and monitor remotely
4. **Customize** - Check GitHub for community customizations

## Support Resources

- **Official Docs**: [M5StickS3 Docs](https://docs.m5stack.com/en/core/StickS3)
- **Klipper**: [klipper3d.org](https://www.klipper3d.org/)
- **Moonraker**: [moonraker.readthedocs.io](https://moonraker.readthedocs.io/)
- **GitHub Issues**: Report bugs and request features

## Enjoy Your Klipper Controller!

Questions? Check the full README.md for detailed documentation.

---

**Device**: M5StickS3 (K150)  
**Firmware**: Klipper Controller v1.0  
**Last Updated**: 2024
