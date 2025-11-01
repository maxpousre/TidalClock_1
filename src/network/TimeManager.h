/**
 * TideClock Time Manager
 *
 * Manages time synchronization with NTP servers and provides
 * time-related utilities for tide data fetching.
 *
 * Phase 3: NOAA Integration
 */

#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <time.h>

class TimeManager {
public:
    /**
     * Initialize time manager with timezone
     * timezone: POSIX timezone string (e.g., "EST5EDT,M3.2.0,M11.1.0")
     */
    static void initialize(const char* timezone);

    /**
     * Synchronize with NTP servers
     * Returns true if sync successful
     * timeoutMs: Maximum time to wait for sync
     */
    static bool syncWithNTP(uint16_t timeoutMs = 10000);

    /**
     * Check if time has been synchronized
     */
    static bool isTimeSynced();

    /**
     * Get current date/time information
     * timeinfo: Output struct tm structure
     */
    static void getCurrentDateTime(struct tm* timeinfo);

    /**
     * Get formatted date string for NOAA API
     * Returns: YYYYMMDD format
     */
    static String getFormattedDate();

    /**
     * Get formatted date/time string for display
     * Returns: YYYY-MM-DD HH:MM:SS format
     */
    static String getFormattedDateTime();

    /**
     * Get current hour (0-23)
     */
    static uint8_t getCurrentHour();

    /**
     * Get current day of month (1-31)
     */
    static uint8_t getCurrentDay();

    /**
     * Get Unix epoch time
     */
    static time_t getEpochTime();

    /**
     * Get formatted time age string
     * seconds: Age in seconds
     * Returns: Human-readable string (e.g., "2 hours ago")
     */
    static String getAgeString(uint32_t seconds);

private:
    static bool timeSynced;
    static char timezoneStr[48];
};

#endif // TIME_MANAGER_H
