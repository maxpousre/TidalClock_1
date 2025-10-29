# TideClock - Product Requirements Document

## 1. Project Overview

**Purpose:** A kinetic art installation that physically displays real-time tide predictions using 24 DC motors, each representing one hour of the day.

**Core Concept:** Motors move to positions corresponding to predicted tide heights, creating a living sculpture that visualizes the tidal cycle.

**Hardware:** 
- ESP32 DEVKIT V1 controller
- 24× N20 6V 120RPM DC motors
- 12× DRV8833 dual H-bridge drivers
- 5× MCP23017 I2C GPIO expanders
- 24× normally-open limit switches

**Operation:** Motors run sequentially (one at a time) to minimize power and create visual interest.

---

## 2. Core Features & User Flow

### Primary Features
1. **Automatic Tide Data Fetch** - Daily retrieval from NOAA API
2. **Motor Homing** - Return all motors to zero position via limit switches
3. **Position Programming** - Move motors to tide-based positions
4. **Web Interface** - Configuration and control via WiFi
5. **Manual Operations** - On-demand fetch and motor testing

### User Flow

```
System Boot
    ↓
WiFi Connection (Station mode, fallback to AP)
    ↓
Web UI Available
    ↓
[Option A: Automatic]          [Option B: Manual]
    ↓                               ↓
Scheduled Fetch Time           User clicks "Fetch & Run"
    ↓                               ↓
    └───────────────┬───────────────┘
                    ↓
            Fetch NOAA Data (24 hours)
                    ↓
            Scale to Motor Times (0-9000ms)
                    ↓
            Home All Motors (sequential)
                    ↓
            Position Motors (sequential)
                    ↓
            Ready for Next Cycle
```

### State Machine
- `BOOT` → `READY` → `FETCHING_DATA` → `HOMING` → `RUNNING_PROGRAM` → `READY`
- `EMERGENCY_STOP` accessible from any state
- `ERROR` state for critical failures

---

## 3. Configuration Examples

### NOAA Station Configuration
```cpp
stationID = "8729108"           // Pensacola, FL
minTideHeight = 0.0             // feet (MLLW datum)
maxTideHeight = 6.0             // feet
```

### Timing Configuration
```cpp
fetchHour = 6                   // 6:00 AM daily fetch
autoFetchEnabled = true
switchReleaseTime = 200         // ms to back away from switch
maxRunTime = 9000               // ms maximum motor runtime
pauseBetweenMotors = 1000       // ms delay between sequential motors
```

### Motor Calibration
```cpp
motorOffsets[24] = {1.0, 1.05, 0.99, ...}  // Per-motor multipliers (0.8-1.2)
```

### WiFi Configuration
```cpp
wifiSSID = "YourNetwork"
wifiPassword = "YourPassword"
// AP fallback: SSID="TideClock", no password, IP=192.168.4.1
```

---

## 4. Data Requirements & System Variables

### NOAA API Request
```
Endpoint: https://api.tidesandcurrents.noaa.gov/api/prod/datagetter
Parameters:
  product=predictions
  station={stationID}
  begin_date={YYYYMMDD}
  end_date={YYYYMMDD}
  datum=MLLW
  time_zone=lst_ldt
  units=english
  interval=h
  format=json
```

### Data Structure
```cpp
struct HourlyTideData {
  uint8_t hour;                // 0-23
  float rawTideHeight;         // feet (MLLW)
  uint16_t scaledRunTime;      // milliseconds (0-9000)
  float offsetMultiplier;      // calibration factor
  uint16_t finalRunTime;       // scaledTime × offset
};
HourlyTideData tideData[24];
```

### Scaling Formula
```cpp
tideRange = maxTideHeight - minTideHeight;
normalizedTide = (rawTide - minTideHeight) / tideRange;
scaledTime = normalizedTide × maxRunTime;
// Clamp: 0 ≤ scaledTime ≤ maxRunTime
finalTime = scaledTime × motorOffsets[hour];
```

### System State
```cpp
enum SystemState {
  STATE_BOOT,
  STATE_READY,
  STATE_FETCHING_DATA,
  STATE_HOMING,
  STATE_RUNNING_PROGRAM,
  STATE_TESTING,
  STATE_EMERGENCY_STOP,
  STATE_ERROR
};
```

### EEPROM Configuration (Persistent)
```cpp
struct PersistentConfig {
  char magic[4];               // "TIDE" validator
  char wifiSSID[32];
  char wifiPassword[64];
  char stationID[10];
  float minTideHeight;
  float maxTideHeight;
  uint8_t fetchHour;
  bool autoFetchEnabled;
  float motorOffsets[24];
  uint16_t switchReleaseTime;
  uint16_t maxRunTime;
  uint16_t checksum;
};
```

### I2C Addresses
```cpp
#define MCP_MOTOR_0   0x20     // Motors 0-7 control
#define MCP_MOTOR_1   0x21     // Motors 8-15 control
#define MCP_MOTOR_2   0x22     // Motors 16-23 control
#define MCP_SWITCH_0  0x23     // Switches 0-15 input
#define MCP_SWITCH_1  0x24     // Switches 16-23 input
```

### Pin Mapping Pattern
Each motor uses 2 pins (IN1, IN2):
- Motor 0: Board 0, GPA0-GPA1
- Motor 1: Board 0, GPA2-GPA3
- Motor 8: Board 1, GPA0-GPA1
- (2 pins per motor × 24 motors = 48 pins across 3 boards)

Each switch uses 1 pin:
- Switch 0: Board 3, GPA0
- Switch 16: Board 4, GPA0

---

## 5. Key Screens & Components

### Tab 1: Main Control
**Components:**
- System status display (state, timestamp, station)
- Tide data table (24 rows: hour | raw tide | motor time | offset | final time)
- `[Fetch & Run Now]` button
- `[Reset to Home]` button
- `[EMERGENCY STOP]` button (large, red, always active)

**States:**
- Fetch & Run: Enabled when `STATE_READY`, disabled during operations
- Emergency Stop: Always enabled

### Tab 2: Advanced
**Components:**
- Motor offset inputs (24 fields, range 0.8-1.2)
- `[Save Offsets]` and `[Reset to 1.0]` buttons
- Motor testing section:
  - Motor selector dropdown (0-23)
  - Run time input (0-9000ms)
  - `[Run Forward]`, `[Run Reverse]`, `[Stop]` buttons
- Limit switch monitor (real-time 24-switch display)

### Tab 3: Configuration
**Components:**
- Station ID input
- Tide range inputs (min/max in feet)
- Auto-fetch checkbox and hour selector
- Motor behavior settings (max run time, switch release, pause duration)
- `[Save Configuration]` button

### Tab 4: Status & Logs
**Components:**
- WiFi connection info (SSID, IP, signal)
- Data status (last fetch, next fetch, validity)
- System log (scrollable, 50-message buffer, auto-refresh 500ms)
- Hardware status (I2C devices, uptime, free memory)

### API Endpoints
```
GET  /                      → Main HTML page
GET  /api/status            → JSON system status
POST /api/fetch             → Trigger fetch & run
POST /api/home              → Home all motors
POST /api/emergency-stop    → Immediate halt
POST /api/test-motor        → Single motor test
GET  /api/switches          → Switch states
POST /api/save-config       → Save to EEPROM
POST /api/save-offsets      → Save motor offsets
```

---

## 6. Technology Stack

### Hardware Platform
- **MCU:** ESP32 DEVKIT V1 (30-pin)
- **Communication:** I2C (GPIO 21=SDA, 22=SCL, 100kHz)
- **Power:** Variable 2.7-6V supply (3A+) with 1A fuse protection

### Software Libraries
```ini
[platformio.ini]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    adafruit/Adafruit MCP23017 Arduino Library @ ^2.3.0
    bblanchon/ArduinoJson @ ^6.21.3
```

### Core Libraries
```cpp
#include <WiFi.h>              // Built-in ESP32 WiFi
#include <WebServer.h>         // Built-in HTTP server
#include <EEPROM.h>            // Built-in persistent storage
#include <Wire.h>              // Built-in I2C
#include <HTTPClient.h>        // Built-in HTTP client
#include <ArduinoJson.h>       // JSON parsing
#include <Adafruit_MCP23X17.h> // GPIO expander
```

### Key Constants
```cpp
#define I2C_SDA 21
#define I2C_SCL 22
#define I2C_FREQ 100000
#define WEB_SERVER_PORT 80
#define SERIAL_BAUD_RATE 115200
#define HOMING_TIMEOUT 9000
#define API_TIMEOUT 15000
#define WIFI_CONNECT_TIMEOUT 20000
#define STATUS_UPDATE_INTERVAL 500
```

---

## 7. Critical Algorithms

### Homing Sequence (per motor)
```cpp
1. Read limit switch state
2. If already triggered: run forward 500ms to release
3. Run motor reverse at full speed
4. Poll switch every 10ms (timeout at 9000ms)
5. When triggered: stop motor
6. Run forward for switchReleaseTime (200ms)
7. Verify switch released
8. Proceed to next motor
```

### Position Program (per motor)
```cpp
1. Get finalRunTime from tideData[motorIndex]
2. If time > 0:
   - Set motor forward (IN1=HIGH, IN2=LOW via I2C)
   - Delay for finalRunTime
   - Stop motor (IN1=LOW, IN2=LOW)
3. Pause pauseBetweenMotors (1000ms)
4. Proceed to next motor
```

### Error Handling
- **I2C Errors:** Retry 3× with 100ms delay, set STATE_ERROR if persistent
- **API Timeouts:** Retry 3× with exponential backoff (5s, 10s, 20s), keep existing data
- **Invalid Data:** Reject entire dataset if < 24 hours, log specific validation failure
- **Homing Timeout:** Stop motor, log error, enter STATE_ERROR, require user reset
- **Emergency Stop:** Immediate motor halt, set flag, disable operations until reset

---

## 8. Implementation Priority

### Phase 1: Core Hardware (Essential)
- I2C communication with MCP23017 boards
- Individual motor control functions
- Limit switch reading
- Basic motor homing

### Phase 2: Web Interface (Essential)
- WiFi connection (Station + AP fallback)
- Basic HTML UI with tabs
- Manual control endpoints
- Status display

### Phase 3: NOAA Integration (Essential)
- HTTP API client
- JSON parsing and validation
- Data scaling algorithm
- Error handling

### Phase 4: Automation (Essential)
- Complete homing sequence
- Position program execution
- Automatic scheduling
- State machine

### Phase 5: Advanced Features (Important)
- Motor offset calibration
- EEPROM configuration storage
- Motor testing interface
- Limit switch monitor

### Phase 6: Polish (Nice-to-Have)
- Enhanced UI styling
- Comprehensive logging
- Configuration backup/restore
- Long-term reliability testing

---

## 9. Success Criteria

- ✓ System fetches tide data automatically once per day
- ✓ All 24 motors home successfully using limit switches
- ✓ Motors position accurately based on tide heights (±50ms timing)
- ✓ Web interface is responsive and intuitive
- ✓ System operates autonomously for 7+ days without intervention
- ✓ Emergency stop halts operations within 100ms
- ✓ Configuration persists across power cycles

---

## 10. Key Constraints & Assumptions

**Constraints:**
- Sequential operation only (one motor at a time)
- Digital control (HIGH/LOW, no PWM)
- 2.4GHz WiFi only
- No authentication (local network only)
- English units (feet) for tide data

**Assumptions:**
- Stable 3.3V power to ESP32
- I2C bus length < 1 meter
- WiFi network available at installation site
- NOAA API remains accessible and stable
- N20 motors draw < 500mA each
- Mechanical design allows sequential operation

---

**Document Version:** 1.0  
**Date:** October 28, 2025  
**Status:** Ready for Implementation
