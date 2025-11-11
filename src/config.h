/**
 * TideClock Configuration Header
 *
 * Defines all hardware constants, pin mappings, and system parameters
 * for the 24-motor tide display system.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// I2C CONFIGURATION
// ============================================================================

#define I2C_SDA 21              // ESP32 I2C Data pin
#define I2C_SCL 22              // ESP32 I2C Clock pin
#define I2C_FREQ 100000         // 100kHz I2C bus speed

// MCP23017 I2C Addresses
#define MCP_MOTOR_0   0x20      // Motors 0-7 control (GPA0-GPB7)
#define MCP_MOTOR_1   0x21      // Motors 8-15 control (GPA0-GPB7)
#define MCP_MOTOR_2   0x22      // Motors 16-23 control (GPA0-GPB7)
#define MCP_SWITCH_0  0x23      // Switches 12-24 input (GPA0-GPB7)
#define MCP_SWITCH_1  0x24      // Switches 0-11 input (GPA0-GPA7)

// I2C Error Handling
#define I2C_RETRY_ATTEMPTS 3
#define I2C_RETRY_DELAY_MS 100

// ============================================================================
// MOTOR SYSTEM CONFIGURATION
// ============================================================================

#define NUM_MOTORS 24           // Total number of motors in system
#define NUM_MOTOR_BOARDS 3      // Number of MCP23017 boards for motor control
#define NUM_SWITCH_BOARDS 2     // Number of MCP23017 boards for switch reading

// Motor Timing Parameters
#define HOMING_TIMEOUT_MS 9000          // Maximum time for homing operation
#define SWITCH_RELEASE_TIME_MS 200      // Time to back away from switch after trigger
#define SWITCH_RELEASE_INITIAL_MS 500   // Time to release if switch already triggered
#define MAX_RUN_TIME_MS 9000            // Maximum motor runtime for positioning
#define PAUSE_BETWEEN_MOTORS_MS 1000    // Delay between sequential motor operations
#define SWITCH_POLL_INTERVAL_MS 10      // How often to check switch during homing

// ============================================================================
// PIN MAPPING STRUCTURES
// ============================================================================

/**
 * Motor Pin Mapping
 * Each motor requires 2 pins (IN1, IN2) on DRV8833 H-bridge
 * Pattern: Motor N uses pins (N*2) and (N*2+1) on its assigned MCP board
 *
 * Motor 0-7:   MCP 0x20 (Board 0)
 * Motor 8-15:  MCP 0x21 (Board 1)
 * Motor 16-23: MCP 0x22 (Board 2)
 */
struct MotorPinMap {
    uint8_t mcpAddress;     // Which MCP23017 board
    uint8_t in1Pin;         // First control pin (forward)
    uint8_t in2Pin;         // Second control pin (reverse)
};

// Motor to MCP board mapping lookup table
const MotorPinMap MOTOR_PIN_MAP[NUM_MOTORS] = {
    // Motors 0-7 on Board 0 (0x20)
    {MCP_MOTOR_0, 0, 1},    // Motor 0: GPA0, GPA1
    {MCP_MOTOR_0, 2, 3},    // Motor 1: GPA2, GPA3
    {MCP_MOTOR_0, 4, 5},    // Motor 2: GPA4, GPA5
    {MCP_MOTOR_0, 6, 7},    // Motor 3: GPA6, GPA7
    {MCP_MOTOR_0, 8, 9},    // Motor 4: GPB0, GPB1
    {MCP_MOTOR_0, 10, 11},  // Motor 5: GPB2, GPB3
    {MCP_MOTOR_0, 12, 13},  // Motor 6: GPB4, GPB5
    {MCP_MOTOR_0, 14, 15},  // Motor 7: GPB6, GPB7

    // Motors 8-15 on Board 1 (0x21)
    {MCP_MOTOR_1, 0, 1},    // Motor 8: GPA0, GPA1
    {MCP_MOTOR_1, 2, 3},    // Motor 9: GPA2, GPA3
    {MCP_MOTOR_1, 4, 5},    // Motor 10: GPA4, GPA5
    {MCP_MOTOR_1, 6, 7},    // Motor 11: GPA6, GPA7
    {MCP_MOTOR_1, 8, 9},    // Motor 12: GPB0, GPB1
    {MCP_MOTOR_1, 10, 11},  // Motor 13: GPB2, GPB3
    {MCP_MOTOR_1, 12, 13},  // Motor 14: GPB4, GPB5
    {MCP_MOTOR_1, 14, 15},  // Motor 15: GPB6, GPB7

    // Motors 16-23 on Board 2 (0x22)
    {MCP_MOTOR_2, 0, 1},    // Motor 16: GPA0, GPA1
    {MCP_MOTOR_2, 2, 3},    // Motor 17: GPA2, GPA3
    {MCP_MOTOR_2, 4, 5},    // Motor 18: GPA4, GPA5
    {MCP_MOTOR_2, 6, 7},    // Motor 19: GPA6, GPA7
    {MCP_MOTOR_2, 8, 9},    // Motor 20: GPB0, GPB1
    {MCP_MOTOR_2, 10, 11},  // Motor 21: GPB2, GPB3
    {MCP_MOTOR_2, 12, 13},  // Motor 22: GPB4, GPB5
    {MCP_MOTOR_2, 14, 15}   // Motor 23: GPB6, GPB7
};

/**
 * Switch Pin Mapping
 * Each switch uses 1 input pin on MCP23017
 *
 * Switch 12-23:  MCP 0x23 (Board 3)
 * Switch 0-11: MCP 0x24 (Board 4)
 */
struct SwitchPinMap {
    uint8_t mcpAddress;     // Which MCP23017 board
    uint8_t pin;            // Input pin number
};

// Switch to MCP board mapping lookup table
const SwitchPinMap SWITCH_PIN_MAP[NUM_MOTORS] = {
    // Switches 0-15 on Board 3 (0x23)
   // {MCP_SWITCH_0, 0},      // Switch 0: GPA0
    //{MCP_SWITCH_0, 1},      // Switch 1: GPA1
    //{MCP_SWITCH_0, 2},      // Switch 2: GPA2
    //{MCP_SWITCH_0, 3},      // Switch 3: GPA3
    //{MCP_SWITCH_0, 4},      // Switch 4: GPA4
    //{MCP_SWITCH_0, 5},      // Switch 5: GPA5
    //{MCP_SWITCH_0, 6},      // Switch 6: GPA6
    //{MCP_SWITCH_0, 7},      // Switch 7: GPA7
    //{MCP_SWITCH_0, 8},      // Switch 8: GPB0
    //{MCP_SWITCH_0, 9},      // Switch 9: GPB1
    //{MCP_SWITCH_0, 10},     // Switch 10: GPB2
    //{MCP_SWITCH_0, 11},     // Switch 11: GPB3
    //{MCP_SWITCH_0, 12},     // Switch 12: GPB4
    //{MCP_SWITCH_0, 13},     // Switch 13: GPB5
    //{MCP_SWITCH_0, 14},     // Switch 14: GPB6
    //{MCP_SWITCH_0, 15},     // Switch 15: GPB7

    // Switches 16-23 on Board 4 (0x24)
    //{MCP_SWITCH_1, 0},      // Switch 16: GPA0
    //{MCP_SWITCH_1, 1},      // Switch 17: GPA1
    //{MCP_SWITCH_1, 2},      // Switch 18: GPA2
    //{MCP_SWITCH_1, 3},      // Switch 19: GPA3
    //{MCP_SWITCH_1, 4},      // Switch 20: GPA4
    //{MCP_SWITCH_1, 5},      // Switch 21: GPA5
    //{MCP_SWITCH_1, 6},      // Switch 22: GPA6
    //{MCP_SWITCH_1, 7}       // Switch 23: GPA7

    // Switches 0-12 on Board 4 (0x24)
    {MCP_SWITCH_1, 13},      // Switch 00: GPB5
    {MCP_SWITCH_1, 12},      // Switch 01: GPB4
    {MCP_SWITCH_1, 11},      // Switch 02: GPB3
    {MCP_SWITCH_1, 10},      // Switch 03: GPB2
    {MCP_SWITCH_1, 9},      // Switch 04: GPB1
    {MCP_SWITCH_1, 8},      // Switch 05: GPB0
    {MCP_SWITCH_1, 0},      // Switch 06: GPA0
    {MCP_SWITCH_1, 1},       // Switch 07: GPA1
    {MCP_SWITCH_1, 2},      // Switch 08: GPA2
    {MCP_SWITCH_1, 3},      // Switch 09: GPA3
    {MCP_SWITCH_1, 4},      // Switch 10: GPA4
    {MCP_SWITCH_1, 5},      // Switch 11: GPA5

    // Switches 0-15 on Board 3 (0x23)
    {MCP_SWITCH_0, 14},      // Switch 12: GPB6
    {MCP_SWITCH_0, 13},      // Switch 13: GPB5
    {MCP_SWITCH_0, 12},      // Switch 14: GPB4
    {MCP_SWITCH_0, 11},      // Switch 15: GPB3
    {MCP_SWITCH_0, 10},      // Switch 16: GPB2
    {MCP_SWITCH_0, 9},      // Switch 17: GPB1
    {MCP_SWITCH_0, 8},      // Switch 18: GPB0
    {MCP_SWITCH_0, 0},      // Switch 19: GPA0
    {MCP_SWITCH_0, 1},      // Switch 20: GPA1
    {MCP_SWITCH_0, 2},      // Switch 21: GPA2
    {MCP_SWITCH_0, 3},     // Switch 22: GPA3
    {MCP_SWITCH_0, 4},     // Switch 23: GPA4
};

// ============================================================================
// WIFI CONFIGURATION
// ============================================================================

#define WIFI_CONNECT_TIMEOUT 20000      // 20 seconds per connection attempt
#define WIFI_MAX_RETRIES 3              // Number of connection attempts before AP mode
#define AP_SSID "TideClock"             // AP mode network name
#define AP_PASSWORD ""                  // AP mode password (empty = open network)
#define AP_IP_ADDRESS "192.168.4.1"     // AP mode IP address

// ============================================================================
// WEB SERVER CONFIGURATION
// ============================================================================

#define WEB_SERVER_PORT 80              // HTTP server port
#define STATUS_UPDATE_INTERVAL 500      // Web UI status refresh rate (ms)
#define LOG_BUFFER_SIZE 50              // Number of log messages to buffer for web UI

// ============================================================================
// EEPROM CONFIGURATION
// ============================================================================

#define EEPROM_SIZE 512                 // Total EEPROM size to allocate
#define CONFIG_MAGIC "TIDE"             // Magic string to validate EEPROM data

// ============================================================================
// SERIAL COMMUNICATION
// ============================================================================

#define SERIAL_BAUD_RATE 115200

// ============================================================================
// DEBUG SETTINGS
// ============================================================================

// Debug verbosity levels (can be set via platformio.ini)
#ifndef DEBUG_MODE
#define DEBUG_MODE 1            // 0=off, 1=on
#endif

#define DEBUG_I2C 1             // Enable I2C communication debugging
#define DEBUG_MOTOR 1           // Enable motor operation debugging
#define DEBUG_SWITCH 1          // Enable switch reading debugging
#define DEBUG_HOMING 1          // Enable homing sequence debugging

#endif // CONFIG_H
