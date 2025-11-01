/**
 * TideClock Tide Data Structures
 *
 * Data structures and manager for storing and managing
 * 24-hour tide predictions from NOAA.
 *
 * Phase 3: NOAA Integration
 */

#ifndef TIDE_DATA_H
#define TIDE_DATA_H

#include <Arduino.h>
#include <time.h>

/**
 * Hourly tide data entry
 * Stores raw tide height and calculated motor run times
 */
struct HourlyTideData {
    uint8_t hour;                    // Hour of day (0-23)
    char timestamp[20];              // ISO format: "2025-11-01 14:00"
    float rawTideHeight;             // Tide height in feet (MLLW datum)
    uint16_t scaledRunTime;          // Base motor run time (0-9000 ms)
    uint16_t finalRunTime;           // After motor offset applied (0-9000 ms)
};

/**
 * Complete 24-hour tide dataset
 * Contains all hourly data plus metadata
 */
struct TideDataset {
    HourlyTideData hours[24];        // 24 hourly entries
    char stationID[10];              // NOAA station ID
    char stationName[64];            // Station name (from NOAA response)
    time_t fetchTime;                // Unix timestamp of fetch
    bool isValid;                    // Data validity flag
    uint8_t recordCount;             // Number of valid records (should be 24)
    char errorMessage[128];          // Last error message (if fetch failed)
};

/**
 * Tide Data Manager
 * Manages in-memory storage and access to tide data
 */
class TideDataManager {
public:
    /**
     * Clear all tide data and reset to invalid state
     */
    static void clear();

    /**
     * Check if current data is valid
     */
    static bool isDataValid();

    /**
     * Check if data is stale (older than maxAgeSeconds)
     * Default: 48 hours
     */
    static bool isDataStale(uint32_t maxAgeSeconds = 172800);

    /**
     * Get data for specific hour
     * Returns nullptr if invalid hour or no data
     */
    static HourlyTideData* getHourData(uint8_t hour);

    /**
     * Get entire dataset (read-only access)
     */
    static const TideDataset* getCurrentDataset();

    /**
     * Get mutable dataset pointer (for updates)
     */
    static TideDataset* getMutableDataset();

    /**
     * Set new tide data
     * Copies data into internal storage
     */
    static void setData(const TideDataset* newData);

    /**
     * Get age of current data in seconds
     * Returns 0 if no valid data
     */
    static uint32_t getDataAgeSeconds();

    /**
     * Get human-readable age string
     * e.g., "2 hours ago"
     */
    static String getDataAgeString();

    /**
     * Set error message for failed fetch
     */
    static void setError(const char* errorMsg);

    /**
     * Get last error message
     */
    static const char* getLastError();

private:
    static TideDataset currentData;
};

#endif // TIDE_DATA_H
