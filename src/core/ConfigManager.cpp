/**
 * TideClock Configuration Manager Implementation
 */

#include "ConfigManager.h"
#include "../config.h"
#include "../utils/Logger.h"
#include <EEPROM.h>
#include <string.h>

// Static member initialization
TideClockConfig ConfigManager::config;
bool ConfigManager::configLoaded = false;

bool ConfigManager::begin() {
    Logger::info(CAT_SYSTEM, "Initializing Configuration Manager...");

    // Initialize EEPROM
    if (!EEPROM.begin(EEPROM_SIZE)) {
        Logger::error(CAT_SYSTEM, "EEPROM initialization failed!");
        setDefaults();
        return false;
    }

    // Try to load existing config
    if (load()) {
        Logger::info(CAT_SYSTEM, "Configuration loaded from EEPROM");
        configLoaded = true;
        return true;
    } else {
        Logger::warning(CAT_SYSTEM, "No valid configuration found - using defaults");
        setDefaults();
        save();  // Save defaults to EEPROM
        return false;
    }
}

bool ConfigManager::load() {
    // Read configuration from EEPROM
    EEPROM.get(0, config);

    // Validate magic string
    if (strncmp(config.magic, CONFIG_MAGIC, 4) != 0) {
        Logger::warning(CAT_SYSTEM, "Invalid magic string in EEPROM");
        return false;
    }

    // Validate checksum
    uint16_t calculatedChecksum = calculateChecksum();
    if (config.checksum != calculatedChecksum) {
        Logger::logf(LOG_WARNING, CAT_SYSTEM,
                     "Checksum mismatch: expected %u, got %u",
                     calculatedChecksum, config.checksum);
        return false;
    }

    // Validate ranges
    if (config.switchReleaseTime < 100 || config.switchReleaseTime > 500) {
        Logger::warning(CAT_SYSTEM, "Invalid switch release time");
        return false;
    }

    if (config.maxRunTime < 1000 || config.maxRunTime > 9000) {
        Logger::warning(CAT_SYSTEM, "Invalid max run time");
        return false;
    }

    return true;
}

bool ConfigManager::save() {
    Logger::info(CAT_SYSTEM, "Saving configuration to EEPROM...");

    // Update checksum
    config.checksum = calculateChecksum();

    // Write to EEPROM
    EEPROM.put(0, config);

    // Commit changes (required for ESP32)
    if (!EEPROM.commit()) {
        Logger::error(CAT_SYSTEM, "EEPROM commit failed!");
        return false;
    }

    Logger::info(CAT_SYSTEM, "Configuration saved successfully");
    return true;
}

void ConfigManager::factoryReset() {
    Logger::warning(CAT_SYSTEM, "Factory reset - restoring defaults");
    setDefaults();
    save();
}

const TideClockConfig& ConfigManager::getConfig() {
    return config;
}

void ConfigManager::setWiFiCredentials(const char* ssid, const char* password) {
    strncpy(config.wifiSSID, ssid, sizeof(config.wifiSSID) - 1);
    config.wifiSSID[sizeof(config.wifiSSID) - 1] = '\0';

    strncpy(config.wifiPassword, password, sizeof(config.wifiPassword) - 1);
    config.wifiPassword[sizeof(config.wifiPassword) - 1] = '\0';

    Logger::logf(LOG_INFO, CAT_SYSTEM, "WiFi credentials updated: SSID=%s", ssid);
}

void ConfigManager::setMotorTiming(uint16_t switchRelease, uint16_t maxRun) {
    // Validate ranges
    if (switchRelease < 100 || switchRelease > 500) {
        Logger::warning(CAT_SYSTEM, "Switch release time out of range (100-500ms)");
        return;
    }

    if (maxRun < 1000 || maxRun > 9000) {
        Logger::warning(CAT_SYSTEM, "Max run time out of range (1000-9000ms)");
        return;
    }

    config.switchReleaseTime = switchRelease;
    config.maxRunTime = maxRun;

    Logger::logf(LOG_INFO, CAT_SYSTEM,
                 "Motor timing updated: switch=%ums, maxRun=%ums",
                 switchRelease, maxRun);
}

void ConfigManager::setNOAAStation(const char* stationID) {
    if (stationID == nullptr || strlen(stationID) == 0) {
        Logger::warning(CAT_SYSTEM, "Station ID cannot be empty");
        return;
    }

    strncpy(config.stationID, stationID, sizeof(config.stationID) - 1);
    config.stationID[sizeof(config.stationID) - 1] = '\0';

    Logger::logf(LOG_INFO, CAT_SYSTEM, "NOAA station ID updated: %s", stationID);
}

void ConfigManager::setTideRange(float minHeight, float maxHeight) {
    // Validate ranges
    if (minHeight >= maxHeight) {
        Logger::warning(CAT_SYSTEM, "Invalid tide range - min must be less than max");
        return;
    }

    if (minHeight < -5.0 || maxHeight > 50.0) {
        Logger::warning(CAT_SYSTEM, "Tide range out of reasonable bounds (-5 to 50 feet)");
        return;
    }

    config.minTideHeight = minHeight;
    config.maxTideHeight = maxHeight;

    Logger::logf(LOG_INFO, CAT_SYSTEM,
                 "Tide range updated: %.1f to %.1f feet",
                 minHeight, maxHeight);
}

void ConfigManager::setMotorOffset(uint8_t motorIndex, float offset) {
    if (motorIndex >= 24) {
        Logger::warning(CAT_SYSTEM, "Invalid motor index");
        return;
    }

    if (offset < 0.8 || offset > 1.2) {
        Logger::warning(CAT_SYSTEM, "Motor offset out of range (0.8-1.2)");
        return;
    }

    config.motorOffsets[motorIndex] = offset;

    Logger::logf(LOG_INFO, CAT_SYSTEM,
                 "Motor %u offset updated: %.3f",
                 motorIndex, offset);
}

float ConfigManager::getMotorOffset(uint8_t motorIndex) {
    if (motorIndex >= 24) {
        return 1.0;  // Default if invalid
    }
    return config.motorOffsets[motorIndex];
}

void ConfigManager::resetMotorOffsets() {
    Logger::info(CAT_SYSTEM, "Resetting all motor offsets to 1.0");
    for (uint8_t i = 0; i < 24; i++) {
        config.motorOffsets[i] = 1.0;
    }
}

void ConfigManager::setAutoFetch(bool enabled, uint8_t hour) {
    if (hour > 23) {
        Logger::warning(CAT_SYSTEM, "Invalid fetch hour (0-23)");
        return;
    }

    config.autoFetchEnabled = enabled;
    config.fetchHour = hour;

    Logger::logf(LOG_INFO, CAT_SYSTEM,
                 "Auto-fetch %s (hour: %u)",
                 enabled ? "enabled" : "disabled", hour);
}

void ConfigManager::setLEDEnabled(bool enabled) {
    config.ledEnabled = enabled;
    Logger::logf(LOG_INFO, CAT_SYSTEM, "LED system %s", enabled ? "enabled" : "disabled");
}

void ConfigManager::setLEDPin(uint8_t pin) {
    // Validate GPIO pin range for ESP32
    if (pin > 39) {
        Logger::warning(CAT_SYSTEM, "Invalid GPIO pin (0-39)");
        return;
    }

    // Warn about reserved pins
    if (pin == 21 || pin == 22) {
        Logger::warning(CAT_SYSTEM, "Warning: GPIO pin conflicts with I2C (21/22)");
    }

    config.ledPin = pin;
    Logger::logf(LOG_INFO, CAT_SYSTEM, "LED data pin set to GPIO %u", pin);
}

void ConfigManager::setLEDCount(uint16_t count) {
    if (count < 1 || count > 300) {
        Logger::warning(CAT_SYSTEM, "LED count out of range (1-300)");
        return;
    }

    config.ledCount = count;
    Logger::logf(LOG_INFO, CAT_SYSTEM, "LED count set to %u", count);
}

void ConfigManager::setLEDMode(uint8_t mode) {
    if (mode > LED_MODE_TEST) {
        Logger::warning(CAT_SYSTEM, "Invalid LED mode");
        return;
    }

    config.ledMode = mode;
    const char* modeName = (mode == LED_MODE_STATIC) ? "Static" : "Test Pattern";
    Logger::logf(LOG_INFO, CAT_SYSTEM, "LED mode set to %s", modeName);
}

void ConfigManager::setLEDBrightness(uint8_t brightness) {
    // Enforce maximum brightness cap (50% = 128)
    if (brightness > LED_MAX_BRIGHTNESS) {
        Logger::logf(LOG_WARNING, CAT_SYSTEM,
                     "Brightness capped at maximum (%u)", LED_MAX_BRIGHTNESS);
        brightness = LED_MAX_BRIGHTNESS;
    }

    config.ledBrightness = brightness;
    Logger::logf(LOG_INFO, CAT_SYSTEM, "LED brightness set to %u (%.0f%%)",
                 brightness, (brightness / 255.0) * 100);
}

void ConfigManager::setLEDColorIndex(uint8_t colorIndex) {
    if (colorIndex > 11) {  // 12 predefined colors (0-11)
        Logger::warning(CAT_SYSTEM, "Invalid color index (0-11)");
        return;
    }

    config.ledColorIndex = colorIndex;
    Logger::logf(LOG_INFO, CAT_SYSTEM, "LED color index set to %u", colorIndex);
}

void ConfigManager::setLEDActiveHours(uint8_t startHour, uint8_t endHour) {
    if (startHour > 23 || endHour > 23) {
        Logger::warning(CAT_SYSTEM, "Invalid active hours (0-23)");
        return;
    }

    config.ledStartHour = startHour;
    config.ledEndHour = endHour;

    Logger::logf(LOG_INFO, CAT_SYSTEM,
                 "LED active hours set to %02u:00 - %02u:00",
                 startHour, endHour);
}

bool ConfigManager::isValid() {
    return configLoaded && (strncmp(config.magic, CONFIG_MAGIC, 4) == 0);
}

void ConfigManager::printConfig() {
    Logger::separator();
    Serial.println("CURRENT CONFIGURATION:");
    Logger::separator();
    Serial.printf("WiFi SSID:           %s\n", config.wifiSSID);
    Serial.printf("WiFi Password:       %s\n",
                  strlen(config.wifiPassword) > 0 ? "********" : "(not set)");
    Serial.printf("Switch Release:      %u ms\n", config.switchReleaseTime);
    Serial.printf("Max Run Time:        %u ms\n", config.maxRunTime);
    Serial.printf("Checksum:            0x%04X\n", config.checksum);
    Logger::separator();
}

uint16_t ConfigManager::calculateChecksum() {
    uint16_t sum = 0;
    uint8_t* data = (uint8_t*)&config;

    // Sum all bytes except the checksum field itself
    size_t checksumOffset = offsetof(TideClockConfig, checksum);

    for (size_t i = 0; i < checksumOffset; i++) {
        sum += data[i];
    }

    return sum;
}

void ConfigManager::setDefaults() {
    Logger::info(CAT_SYSTEM, "Setting default configuration");

    // Set magic string
    strncpy(config.magic, CONFIG_MAGIC, 4);

    // Default WiFi credentials (empty - will use AP mode)
    config.wifiSSID[0] = '\0';
    config.wifiPassword[0] = '\0';

    // Default motor timing from config.h
    config.switchReleaseTime = SWITCH_RELEASE_TIME_MS;
    config.maxRunTime = MAX_RUN_TIME_MS;

    // Phase 3: NOAA defaults
    config.stationID[0] = '\0';         // Empty - user must configure
    config.minTideHeight = 0.0;         // 0 feet MLLW
    config.maxTideHeight = 6.0;         // 6 feet MLLW (typical range)
    config.autoFetchEnabled = false;    // Manual fetch only by default
    config.fetchHour = 0;               // Midnight (if enabled)

    // Initialize all motor offsets to 1.0 (no adjustment)
    for (uint8_t i = 0; i < 24; i++) {
        config.motorOffsets[i] = 1.0;
    }

    // Phase 4: FastLED defaults
    config.ledEnabled = false;                      // LED system off by default
    config.ledPin = LED_DEFAULT_PIN;                // GPIO 15
    config.ledCount = LED_DEFAULT_COUNT;            // 160 LEDs
    config.ledMode = LED_MODE_STATIC;               // Static color mode
    config.ledBrightness = LED_DEFAULT_BRIGHTNESS;  // 20% brightness (51/255)
    config.ledColorIndex = 6;                       // Cyan (ocean theme)
    config.ledStartHour = LED_DEFAULT_START_HOUR;   // 8 AM
    config.ledEndHour = LED_DEFAULT_END_HOUR;       // 10 PM

    // Checksum will be calculated when saved
    config.checksum = 0;

    configLoaded = false;
}
