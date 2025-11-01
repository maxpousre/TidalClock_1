/**
 * TideClock NOAA API Client
 *
 * Handles fetching and parsing tide predictions from
 * NOAA's Tides and Currents API.
 *
 * Phase 3: NOAA Integration
 */

#ifndef NOAA_CLIENT_H
#define NOAA_CLIENT_H

#include <Arduino.h>
#include "../data/TideData.h"

class NOAAClient {
public:
    /**
     * Fetch result codes
     */
    enum FetchResult {
        SUCCESS,              // Data fetched and parsed successfully
        NETWORK_ERROR,        // HTTP connection/request failed
        TIMEOUT,              // Request timed out
        INVALID_STATION,      // Station ID not found or invalid
        PARSE_ERROR,          // JSON parsing failed
        INCOMPLETE_DATA,      // Less than 24 hours of data
        NO_TIME_SYNC,         // System time not synchronized
        CONFIG_ERROR          // Missing or invalid configuration
    };

    /**
     * Fetch tide predictions from NOAA API
     *
     * stationID: NOAA station ID (e.g., "8729108")
     * output: Pointer to TideDataset to populate
     * timeoutMs: HTTP request timeout in milliseconds
     *
     * Returns: FetchResult status code
     */
    static FetchResult fetchTidePredictions(
        const char* stationID,
        TideDataset* output,
        uint16_t timeoutMs = 10000
    );

    /**
     * Get human-readable error message for result code
     */
    static const char* getErrorMessage(FetchResult result);

private:
    /**
     * Build NOAA API request URL
     *
     * stationID: NOAA station ID
     * dateStr: Date in YYYYMMDD format
     *
     * Returns: Complete URL string
     */
    static String buildRequestURL(const char* stationID, const char* dateStr);

    /**
     * Parse NOAA JSON response
     *
     * response: JSON response body from NOAA
     * output: TideDataset to populate
     *
     * Returns: true if parsing successful
     */
    static bool parseJSON(const String& response, TideDataset* output);

    /**
     * Validate tide dataset completeness
     *
     * data: TideDataset to validate
     *
     * Returns: true if data is complete and valid
     */
    static bool validateData(TideDataset* data);

    /**
     * Scale raw tide height to motor run time
     *
     * tideHeight: Tide height in feet (MLLW)
     * minTide: Minimum expected tide (from config)
     * maxTide: Maximum expected tide (from config)
     *
     * Returns: Motor run time in milliseconds (0-9000)
     */
    static uint16_t scaleToRunTime(float tideHeight, float minTide, float maxTide);

    /**
     * Apply motor-specific offset multipliers
     *
     * data: TideDataset with scaledRunTime populated
     *
     * Updates finalRunTime for each hour
     */
    static void applyMotorOffsets(TideDataset* data);

    /**
     * Extract hour from timestamp string
     *
     * timestamp: String in format "YYYY-MM-DD HH:MM"
     *
     * Returns: Hour (0-23) or 255 on error
     */
    static uint8_t extractHour(const char* timestamp);

    /**
     * Make HTTP GET request with retry logic
     *
     * url: Full URL to request
     * response: String to store response body
     * timeoutMs: Request timeout
     *
     * Returns: HTTP response code (200 = success)
     */
    static int httpGetWithRetry(const String& url, String& response, uint16_t timeoutMs);
};

#endif // NOAA_CLIENT_H
