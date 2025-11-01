/**
 * TideClock Main Program
 *
 * Phase 3: NOAA Tide Integration
 * Provides web interface with tide data fetching and motor control.
 */

#include <Arduino.h>
#include "config.h"
#include "utils/Logger.h"
#include "hardware/I2CManager.h"
#include "hardware/GPIOExpander.h"
#include "hardware/SwitchReader.h"
#include "hardware/MotorController.h"
#include "core/StateManager.h"
#include "core/ConfigManager.h"
#include "network/WiFiManager.h"
#include "network/WebServer.h"
#include "network/TimeManager.h"
#include "data/TideData.h"

// Forward declarations
void printHelp();
void processSerialCommand();
void systemInitialization();

void setup() {
    // Initialize serial communication
    Logger::begin();

    // Print boot header
    Logger::printBootHeader();

    // Initialize core systems
    StateManager::begin();
    ConfigManager::begin();

    // Initialize all hardware systems
    systemInitialization();

    // Phase 3: Initialize Time Manager
    Logger::info(CAT_SYSTEM, "Initializing Time Manager...");
    TimeManager::initialize("EST5EDT,M3.2.0,M11.1.0");  // US Eastern Time

    // Phase 3: Initialize Tide Data Manager
    Logger::info(CAT_SYSTEM, "Initializing Tide Data Manager...");
    TideDataManager::clear();

    // Initialize WiFi
    WiFiManager::begin();
    WiFiManager::connect();

    // Phase 3: Sync time with NTP if WiFi connected
    if (WiFiManager::isConnected()) {
        Logger::info(CAT_SYSTEM, "Synchronizing time with NTP servers...");
        if (TimeManager::syncWithNTP(10000)) {
            String dateTime = TimeManager::getFormattedDateTime();
            Logger::logf(LOG_INFO, CAT_SYSTEM, "NTP sync successful: %s", dateTime.c_str());
        } else {
            Logger::warning(CAT_SYSTEM, "NTP sync failed - tide fetch will not work until time is synced");
        }
    } else {
        Logger::warning(CAT_SYSTEM, "WiFi not connected - NTP sync skipped");
    }

    // Start web server
    TideClockWebServer::begin();

    // System ready
    StateManager::setState(STATE_READY);

    // Print help menu for serial interface
    printHelp();

    Logger::separator();
    Logger::info(CAT_SYSTEM, "*** TIDECLOCK PHASE 3 READY ***");
    Logger::logf(LOG_INFO, CAT_SYSTEM, "Web interface: http://%s", WiFiManager::getIPAddress().c_str());
    Logger::info(CAT_SYSTEM, "Serial interface: Active");
    Logger::info(CAT_SYSTEM, "NOAA Integration: Enabled");
    Logger::separator();
}

void loop() {
    // Handle web server requests
    TideClockWebServer::handle();

    // Handle WiFi events
    WiFiManager::handle();

    // Check for serial commands
    if (Serial.available() > 0) {
        processSerialCommand();
    }

    // Small delay to prevent overwhelming the system
    delay(10);
}

void systemInitialization() {
    Logger::info(CAT_SYSTEM, "Starting system initialization...");
    Logger::separator();

    bool allSuccess = true;

    // Step 1: Initialize I2C bus
    if (!I2CManager::begin()) {
        Logger::error(CAT_SYSTEM, "I2C initialization failed!");
        allSuccess = false;
    }

    // Step 2: Scan I2C bus
    I2CManager::printStatus();

    // Step 3: Verify all devices
    if (!I2CManager::verifyAllDevices()) {
        Logger::error(CAT_SYSTEM, "Not all I2C devices found!");
        allSuccess = false;
    }

    // Step 4: Initialize GPIO expanders
    if (!GPIOExpander::begin()) {
        Logger::error(CAT_SYSTEM, "GPIO expander initialization failed!");
        allSuccess = false;
    }

    // Step 5: Initialize switch reader
    if (!SwitchReader::begin()) {
        Logger::error(CAT_SYSTEM, "Switch reader initialization failed!");
        allSuccess = false;
    }

    // Step 6: Initialize motor controller
    if (!MotorController::begin()) {
        Logger::error(CAT_SYSTEM, "Motor controller initialization failed!");
        allSuccess = false;
    }

    Logger::separator();
    if (allSuccess) {
        Logger::info(CAT_SYSTEM, "*** SYSTEM INITIALIZATION COMPLETE ***");
        Logger::info(CAT_SYSTEM, "All systems operational - Ready for testing");
    } else {
        Logger::error(CAT_SYSTEM, "*** INITIALIZATION FAILED ***");
        Logger::error(CAT_SYSTEM, "Some systems failed to initialize - Check connections");
    }
    Logger::separator();
}

void printHelp() {
    Logger::separator();
    Serial.println("TIDECLOCK HARDWARE TEST INTERFACE");
    Logger::separator();
    Serial.println("Motor Control Commands:");
    Serial.println("  h [motor]       - Home specific motor (0-23)");
    Serial.println("  H               - Home all motors sequentially");
    Serial.println("  f [motor] [ms]  - Run motor forward for [ms] milliseconds");
    Serial.println("  r [motor] [ms]  - Run motor reverse for [ms] milliseconds");
    Serial.println("  s [motor]       - Stop specific motor");
    Serial.println("  S               - Emergency stop all motors");
    Serial.println("  C               - Clear emergency stop");
    Serial.println("");
    Serial.println("Switch Reading Commands:");
    Serial.println("  w [switch]      - Read specific switch state (0-23)");
    Serial.println("  W               - Read all switch states");
    Serial.println("");
    Serial.println("I2C Diagnostic Commands:");
    Serial.println("  i               - Scan I2C bus");
    Serial.println("  I               - Full I2C status report");
    Serial.println("  v               - Verify all devices");
    Serial.println("");
    Serial.println("System Commands:");
    Serial.println("  ?               - Print this help menu");
    Serial.println("  R               - Reset system (software restart)");
    Logger::separator();
    Serial.println("Ready for commands. Type ? for help.");
    Logger::separator();
}

void processSerialCommand() {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.length() == 0) {
        return;
    }

    Logger::logf(LOG_INFO, CAT_TEST, "Command received: %s", command.c_str());

    char cmd = command.charAt(0);
    int firstSpace = command.indexOf(' ');
    int secondSpace = command.indexOf(' ', firstSpace + 1);

    int arg1 = -1;
    int arg2 = -1;

    if (firstSpace > 0) {
        arg1 = command.substring(firstSpace + 1, secondSpace > 0 ? secondSpace : command.length()).toInt();
    }

    if (secondSpace > 0) {
        arg2 = command.substring(secondSpace + 1).toInt();
    }

    // Process commands
    switch (cmd) {
        // === MOTOR COMMANDS ===
        case 'h': {  // Home single motor
            if (arg1 < 0 || arg1 >= NUM_MOTORS) {
                Logger::error(CAT_TEST, "Invalid motor index. Use: h [0-23]");
                break;
            }
            HomingResult result = MotorController::homeSingleMotor(arg1);
            Logger::logf(LOG_INFO, CAT_TEST, "Homing result: %s",
                         MotorController::getHomingResultString(result));
            break;
        }

        case 'H': {  // Home all motors
            Logger::info(CAT_TEST, "Starting full homing sequence...");
            uint8_t count = MotorController::homeAllMotors();
            Logger::logf(LOG_INFO, CAT_TEST, "Homed %d/%d motors", count, NUM_MOTORS);
            break;
        }

        case 'f': {  // Run motor forward
            if (arg1 < 0 || arg1 >= NUM_MOTORS || arg2 < 0) {
                Logger::error(CAT_TEST, "Invalid parameters. Use: f [motor] [milliseconds]");
                break;
            }
            MotorController::runMotorForward(arg1, arg2);
            break;
        }

        case 'r': {  // Run motor reverse
            if (arg1 < 0 || arg1 >= NUM_MOTORS || arg2 < 0) {
                Logger::error(CAT_TEST, "Invalid parameters. Use: r [motor] [milliseconds]");
                break;
            }
            MotorController::runMotorReverse(arg1, arg2);
            break;
        }

        case 's': {  // Stop single motor
            if (arg1 < 0 || arg1 >= NUM_MOTORS) {
                Logger::error(CAT_TEST, "Invalid motor index. Use: s [0-23]");
                break;
            }
            MotorController::stopMotor(arg1);
            Logger::logf(LOG_INFO, CAT_TEST, "Motor %d stopped", arg1);
            break;
        }

        case 'S': {  // Emergency stop all
            MotorController::emergencyStopAll();
            break;
        }

        case 'C': {  // Clear emergency stop
            MotorController::clearEmergencyStop();
            break;
        }

        // === SWITCH COMMANDS ===
        case 'w': {  // Read single switch
            if (arg1 < 0 || arg1 >= NUM_MOTORS) {
                Logger::error(CAT_TEST, "Invalid switch index. Use: w [0-23]");
                break;
            }
            bool triggered = SwitchReader::isSwitchTriggered(arg1);
            Logger::logf(LOG_INFO, CAT_TEST, "Switch %d: %s",
                         arg1, SwitchReader::getStateString(triggered));
            break;
        }

        case 'W': {  // Read all switches
            SwitchReader::printAllSwitches();
            break;
        }

        // === I2C COMMANDS ===
        case 'i': {  // I2C scan
            Logger::info(CAT_TEST, "Scanning I2C bus...");
            uint8_t count = I2CManager::scanBus(true);
            Logger::logf(LOG_INFO, CAT_TEST, "Found %d devices", count);
            break;
        }

        case 'I': {  // Full I2C status
            I2CManager::printStatus();
            break;
        }

        case 'v': {  // Verify all devices
            bool allPresent = I2CManager::verifyAllDevices();
            if (allPresent) {
                Logger::info(CAT_TEST, "All devices verified OK");
            } else {
                Logger::error(CAT_TEST, "Some devices missing");
            }
            break;
        }

        // === SYSTEM COMMANDS ===
        case '?': {  // Help
            printHelp();
            break;
        }

        case 'R': {  // Reset
            Logger::warning(CAT_SYSTEM, "Restarting system in 2 seconds...");
            delay(2000);
            ESP.restart();
            break;
        }

        default: {
            Logger::logf(LOG_WARNING, CAT_TEST, "Unknown command: %c", cmd);
            Serial.println("Type ? for help");
            break;
        }
    }
}
