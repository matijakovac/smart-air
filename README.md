# Smart Cellar Ventilation System

A comprehensive ESP32-based ventilation control system with intelligent climate management.

## Features

- ✅ Dual sensor monitoring (internal/external)
- ✅ Intelligent humidity and temperature control
- ✅ Dew point calculation for condensation prevention
- ✅ Time-based scheduling (quiet hours)
- ✅ Three-speed operation (OFF/LOW 60%/HIGH 100%)
- ✅ Forced circulation every 6 hours
- ✅ Web dashboard for monitoring and control
- ✅ Home Assistant MQTT integration
- ✅ OTA firmware updates
- ✅ OLED status display
- ✅ Anti-short-cycle protection

## Hardware Required

- ESP32 DevKit v4
- 2x AHT20+BMP280 sensor modules
- RobotDyn AC Dimmer 8A (or compatible)
- 220V Relay module
- 0.96" SSD1306 OLED Display (I2C)
- LTC4311 I2C Active Terminator
- Cat5 UTP cable (for remote sensor)
- Duct fan (220V AC)

## Pin Connections

```
ESP32 Pin    | Connection
-------------|----------------------------------
GPIO 5       | Relay control (active LOW)
GPIO 16      | External sensor SDA (via LTC4311)
GPIO 17      | External sensor SCL (via LTC4311)
GPIO 18      | Internal sensor SCL
GPIO 19      | Internal sensor SDA
GPIO 21      | Dimmer PSM (trigger output)
GPIO 22      | Dimmer Z-C (zero cross input)
GPIO 13       | OLED SDA
GPIO 12      | OLED SCL
```

## Installation

### 1. Setup Project

```bash
# Create project directory
cd ~/Documents/PlatformIO/Projects
mkdir cellar-ventilation
cd cellar-ventilation

# Create directory structure
mkdir -p src include lib/RBDdimmer data
```

### 2. Copy Files

Copy all the source files provided above into their respective directories:
- `platformio.ini` → root directory
- `include/*.h` → include/
- `src/*.cpp` → src/
- `data/config.json` → data/
- Copy your `rbdimmerESP32.h` and `rbdimmerESP32.cpp` → lib/RBDdimmer/

### 3. Configure WiFi

Edit `data/config.json`:

```json
{
  "wifi": {
    "ssid": "YOUR_ACTUAL_WIFI_NAME",
    "password": "YOUR_ACTUAL_WIFI_PASSWORD",
    "hostname": "cellar-fan"
  },
  ...
}
```

### 4. Build and Upload

```bash
# Install dependencies
pio lib install

# Build the project
pio run

# Upload filesystem (config.json)
pio run --target uploadfs

# Upload firmware
pio run --target upload

# Monitor serial output
pio device monitor
```

## First Time Setup

1. **Connect to Serial Monitor** (115200 baud)
2. **Watch initialization messages**
   - All sensors should show ✓
   - WiFi should connect
   - AC frequency should be detected (50 Hz)
3. **Access Web Interface**
   - Open browser: `http://cellar-fan.local` or `http://[ESP32_IP]`
4. **Verify sensor readings**
   - Check internal and external temperatures
   - Verify humidity readings
5. **Test fan manually**
   - Click "LOW (60%)" button
   - Fan should start at 60% speed
   - Click "HIGH (100%)" for full speed
   - Click "OFF" to stop

## Web Dashboard

Access the web interface at: `http://cellar-fan.local`

Features:
- Real-time sensor readings
- Fan status and speed
- Manual control buttons
- System information
- OTA firmware updates

## Control Modes

### AUTO Mode (Default)
Automatic control based on environmental conditions:
- Monitors temperature and humidity
- Checks dew point safety
- Respects time-based speed limits
- Forced circulation every 6 hours

### Manual Modes
- **OFF**: Fan stopped (temporary override)
- **LOW**: 60% speed (quiet operation)
- **HIGH**: 100% speed (maximum effectiveness)

Manual overrides last 60 minutes by default, then return to AUTO.

## Schedule Logic

**HIGH speed (100%) allowed:**
- Monday-Thursday: 22:00-15:00
- Friday: 22:00-00:00
- Saturday: NEVER
- Sunday: NEVER

**LOW speed (60%):**
- All other times when ventilation is needed

## Home Assistant Integration

### Enable MQTT

Edit `data/config.json`:

```json
{
  "mqtt": {
    "enabled": true,
    "broker": "192.168.1.100",
    "port": 1883,
    "user": "mqtt_user",
    "password": "mqtt_password"
  }
}
```

Re-upload filesystem: `pio run --target uploadfs`

### Auto-Discovery

The system will automatically create Home Assistant entities:
- `sensor.cellar_temperature_internal`
- `sensor.cellar_humidity_internal`
- `sensor.cellar_temperature_external`
- `sensor.cellar_humidity_external`
- `sensor.cellar_fan_speed`

### Control from HA

Publish to MQTT topics:
```bash
# Set mode
mosquitto_pub -h localhost -t "cellar/mode/set" -m "auto"
mosquitto_pub -h localhost -t "cellar/mode/set" -m "low"
mosquitto_pub -h localhost -t "cellar/mode/set" -m "high"
mosquitto_pub -h localhost -t "cellar/mode/set" -m "off"
```

## OTA Updates

### Via Web Interface
1. Go to `http://cellar-fan.local/update`
2. Choose firmware file (`.bin`)
3. Click Upload
4. Wait for reboot

### Via PlatformIO
```bash
# Upload over WiFi (after initial USB upload)
pio run --target upload --upload-port cellar-fan.local
```

## Configuration

All settings can be adjusted in `data/config.json`:

```json
{
  "thresholds": {
    "target_temp": 18.0,           // Desired cellar temperature
    "target_humidity": 55.0,        // Desired humidity level
    "temp_differential": 3.0,       // °C cooler outside to run
    "humidity_differential": 10.0,  // % less humid outside to run
    "min_outside_temp": -5.0,       // Safety: Don't run if too cold
    "min_cellar_temp": 8.0,         // Safety: Don't over-cool cellar
    "max_dew_point_increase": 3.0   // Max dew point rise allowed
  },
  "fan": {
    "low_speed": 60,                // Low speed percentage
    "high_speed": 100,              // High speed percentage
    "min_run_time_sec": 300,        // Min 5 minutes ON
    "min_idle_time_sec": 180        // Min 3 minutes OFF
  },
  "circulation": {
    "forced_interval_hours": 6,     // Force run every X hours
    "forced_duration_min": 10       // Run for X minutes
  }
}
```

After changes:
```bash
pio run --target uploadfs
```

Or edit via web interface (future feature).

## Troubleshooting

### Sensors Not Detected

```
❌ Internal AHT20 not found!
```

**Solution:**
- Check I2C wiring (SDA/SCL)
- Verify power to sensors (3.3V or 5V)
- Check I2C addresses (0x38 for AHT20, 0x76/0x77 for BMP280)

### External Sensor Not Working

```
❌ External AHT20 not found!
```

**Solution:**
- Check Cat5 cable connections
- Verify LTC4311 EN pin is connected to VIN/5V
- Test with sensor connected directly (bypass LTC4311)
- Check voltage at remote end (should be ~3.3-5V)

### Dimmer Not Controlling Fan

**Solution:**
- Verify zero-cross detection (should see "50 Hz detected")
- Check PSM and Z-C pin connections
- Ensure relay is ON (active LOW on GPIO 5)
- Check AC wiring: Relay → Dimmer IN → Dimmer OUT → Fan

### WiFi Not Connecting

**Solution:**
- Verify SSID and password in config.json
- Check WiFi signal strength
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Try mobile hotspot for testing

### OLED Display Blank

**Solution:**
- Check I2C address (usually 0x3C)
- Verify SDA/SCL connections (GPIO 4/15)
- Try different I2C pins if needed
- Check if display is powered (3.3V or 5V depending on module)

### Fan Runs Constantly

**Solution:**
- Check AUTO mode is active (not manual override)
- Verify sensor readings are correct
- Check thresholds in config.json
- Look at serial output for decision logic

## Serial Debug Output

Connect to serial monitor to see detailed logs:

```
📊 Environmental Conditions:
   Internal: 22.3°C, 58%, 981.2 hPa, DP: 13.5°C
   External: 15.1°C, 72%, 982.1 hPa, DP: 10.2°C

🧠 Decision: RUNNING
   Reason: Humidity benefit
   Speed: LOW (60%)
   Next forced run: 127 minutes
```

## Safety Features

1. **Temperature Protection**
   - Won't run if outside < -5°C
   - Won't run if cellar < 8°C

2. **Dew Point Protection**
   - Calculates if incoming air will condense
   - Blocks ventilation if condensation risk

3. **Anti-Short-Cycle**
   - Minimum 5 minutes ON
   - Minimum 3 minutes OFF
   - Prevents relay wear

4. **Forced Circulation**
   - Runs every 6 hours regardless of conditions
   - Prevents stale air buildup

5. **Watchdog Timer**
   - ESP32 auto-resets if system hangs

## Performance Tips

1. **Optimize WiFi**
   - Place ESP32 near router or use WiFi extender
   - Strong signal = faster web interface

2. **Sensor Placement**
   - Internal: Mid-height, away from fan
   - External: Shaded area, protected from rain

3. **Cable Management**
   - Use shielded Cat5 for external sensor
   - Keep power cables away from sensor cables

4. **Maintenance**
   - Clean fan blades monthly
   - Check sensor readings periodically
   - Update firmware when available

## Advanced Features

### Custom Schedules

Edit `fancontrol.cpp` function `isHighSpeedAllowed()` to customize schedule.

### Seasonal Profiles

Modify thresholds based on season (future feature - currently manual).

### Data Logging

Enable MQTT and use Home Assistant's recorder to log all data.

### Alerts

Use Home Assistant automations:
```yaml
automation:
  - alias: "High Cellar Humidity Alert"
    trigger:
      - platform: numeric_state
        entity_id: sensor.cellar_humidity_internal
        above: 70
        for:
          hours: 2
    action:
      - service: notify.mobile_app
        data:
          message: "Cellar humidity is {{ states('sensor.cellar_humidity_internal') }}%"
```

## Version History

- **v1.0.0** (2024-10) - Initial release
  - Basic ventilation control
  - Web interface
  - MQTT support
  - OLED display
  - OTA updates

## License

MIT License - Feel free to modify and distribute.

## Support

For issues or questions:
1. Check troubleshooting section
2. Review serial output logs
3. Check sensor readings on web interface
4. Verify wiring connections

## Credits

- Developed for home automation enthusiasts
- Uses Adafruit sensor libraries
- RBDimmer library for AC control
- AsyncWebServer for ESP32
```

---

## Quick Start Checklist

Save this as a checklist for yourself:

```markdown
# Cellar Ventilation - Setup Checklist

## Hardware Setup
- [ ] ESP32 connected to computer via USB
- [ ] All sensors wired and powered
- [ ] Relay connected to GPIO 5
- [ ] Dimmer connected (PSM=21, ZC=22)
- [ ] OLED display connected (SDA=4, SCL=15)
- [ ] LTC4311 connected with EN to 5V
- [ ] External sensor connected via Cat5

## Software Setup
- [ ] PlatformIO installed in VSCode
- [ ] Project folder created
- [ ] All source files copied
- [ ] RBDdimmer library files in lib/RBDdimmer/
- [ ] config.json edited with WiFi credentials
- [ ] Filesystem uploaded (`pio run --target uploadfs`)
- [ ] Firmware compiled and uploaded

## Testing
- [ ] Serial monitor shows all sensors initialized
- [ ] WiFi connected successfully
- [ ] AC frequency detected (50 Hz)
- [ ] Web interface accessible
- [ ] Internal sensor readings valid
- [ ] External sensor readings valid
- [ ] Fan responds to manual LOW command
- [ ] Fan responds to manual HIGH command
- [ ] OLED display shows correct data

## Final Steps
- [ ] Set mode to AUTO
- [ ] Monitor for 24 hours
- [ ] Verify forced circulation runs every 6h
- [ ] Check schedule compliance
- [ ] Document any custom settings
```

---

That's the complete project! Here's what you need to do:

1. **Create the project structure** in VSCode/PlatformIO
2. **Copy all the files** I've provided above into the correct locations
3. **Copy your RBDimmer library** files into `lib/RBDdimmer/`
4. **Edit `data/config.json`** with your WiFi credentials
5. **Build and upload** using PlatformIO

