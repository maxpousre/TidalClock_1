/**
 * TideClock Configuration Manager
 *
 * Manages persistent configuration storage in EEPROM
 * Phase 2: WiFi credentials and basic motor settings
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>

/**
 * Configuration structure stored in EEPROM
 * Phase 2 subset - will be expanded in later phases
 */
struct TideClockConfig {
    char magic[4];                  // "TIDE" - validates EEPROM data
    char wifiSSID[32];              // WiFi network name
    char wifiPassword[64];          // WiFi password
    uint16_t switchReleaseTime;     // Time to back away from switch (ms)
    uint16_t maxRunTime;            // Maximum motor runtime (ms)

    // Phase 3: NOAA Integration
    char stationID[10];             // NOAA station ID (e.g., "8729108")
    float minTideHeight;            // Expected minimum tide (feet, MLLW)
    float maxTideHeight;            // Expected maximum tide (feet, MLLW)
    float motorOffsets[24];         // Motor-specific calibration multipliers
    bool autoFetchEnabled;          // Enable automatic daily fetch
    uint8_t fetchHour;              // Hour to fetch (0-23, for automatic mode)

    uint16_t checksum;              // Simple checksum for validation
};

class ConfigManager {
public:
    /**
     * Initialize EEPROM and load configuration
     * Returns true if valid config found, false if using defaults
     */
    static bool begin();

    /**
     * Load configuration from EEPROM
     * Returns true if valid config loaded
     */
    static bool load();

    /**
     * Save current configuration to EEPROM
     * Returns true if save successful
     */
    static bool save();

    /**
     * Reset to factory defaults and save
     */
    static void factoryReset();

    /**
     * Get current configuration (read-only access)
     */
    static const TideClockConfig& getConfig();

    /**
     * Update WiFi credentials
     */
    static void setWiFiCredentials(const char* ssid, const char* password);

    /**
     * Update motor timing parameters
     */
    static void setMotorTiming(uint16_t switchRelease, uint16_t maxRun);

    /**
     * Update NOAA station configuration
     */
    static void setNOAAStation(const char* stationID);

    /**
     * Update tide range configuration
     */
    static void setTideRange(float minHeight, float maxHeight);

    /**
     * Update motor offset for specific motor
     */
    static void setMotorOffset(uint8_t motorIndex, float offset);

    /**
     * Get motor offset for specific motor
     */
    static float getMotorOffset(uint8_t motorIndex);

    /**
     * Reset all motor offsets to 1.0
     */
    static void resetMotorOffsets();

    /**
     * Update automatic fetch settings
     */
    static void setAutoFetch(bool enabled, uint8_t hour);

    /**
     * Validation helpers
     */
    static bool isValid();
    static void printConfig();

private:
    static TideClockConfig config;
    static bool configLoaded;

    static uint16_t calculateChecksum();
    static void setDefaults();
};

#endif // CONFIG_MANAGER_H
