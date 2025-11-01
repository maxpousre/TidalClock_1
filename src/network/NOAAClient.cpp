/**
 * TideClock NOAA API Client Implementation
 */

#include "NOAAClient.h"
#include "TimeManager.h"
#include "../core/ConfigManager.h"
#include "../utils/Logger.h"
#include "../config.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

// NOAA API Configuration
#define NOAA_API_BASE "https://api.tidesandcurrents.noaa.gov/api/prod/datagetter"
#define NOAA_RETRY_ATTEMPTS 3
#define NOAA_RETRY_DELAY_MS 2000

NOAAClient::FetchResult NOAAClient::fetchTidePredictions(
    const char* stationID,
    TideDataset* output,
    uint16_t timeoutMs
) {
    // Validate inputs
    if (stationID == nullptr || strlen(stationID) == 0) {
        Logger::error(CAT_SYSTEM, "NOAA: Station ID not provided");
        return CONFIG_ERROR;
    }

    if (output == nullptr) {
        Logger::error(CAT_SYSTEM, "NOAA: Output dataset is null");
        return CONFIG_ERROR;
    }

    // Check time synchronization
    if (!TimeManager::isTimeSynced()) {
        Logger::error(CAT_SYSTEM, "NOAA: Time not synchronized");
        return NO_TIME_SYNC;
    }

    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Logger::error(CAT_SYSTEM, "NOAA: WiFi not connected");
        return NETWORK_ERROR;
    }

    // Clear output dataset
    memset(output, 0, sizeof(TideDataset));
    output->isValid = false;
    output->recordCount = 0;

    // Copy station ID
    strncpy(output->stationID, stationID, sizeof(output->stationID) - 1);
    output->stationID[sizeof(output->stationID) - 1] = '\0';

    // Get current date
    String dateStr = TimeManager::getFormattedDate();
    Logger::logf(LOG_INFO, CAT_SYSTEM,
                "NOAA: Fetching tide data for station %s on %s",
                stationID, dateStr.c_str());

    // Build URL
    String url = buildRequestURL(stationID, dateStr.c_str());
    Logger::logf(LOG_INFO, CAT_SYSTEM, "NOAA: Request URL: %s", url.c_str());

    // Make HTTP request
    String response;
    int httpCode = httpGetWithRetry(url, response, timeoutMs);

    if (httpCode != 200) {
        Logger::logf(LOG_ERROR, CAT_SYSTEM,
                    "NOAA: HTTP request failed with code %d", httpCode);

        if (httpCode == 404) {
            return INVALID_STATION;
        } else if (httpCode < 0) {
            return TIMEOUT;
        } else {
            return NETWORK_ERROR;
        }
    }

    Logger::logf(LOG_INFO, CAT_SYSTEM,
                "NOAA: Received %u bytes of data", response.length());

    // Parse JSON response
    if (!parseJSON(response, output)) {
        Logger::error(CAT_SYSTEM, "NOAA: JSON parsing failed");
        return PARSE_ERROR;
    }

    // Validate data completeness
    if (!validateData(output)) {
        Logger::error(CAT_SYSTEM, "NOAA: Data validation failed");
        return INCOMPLETE_DATA;
    }

    // Apply motor offsets
    applyMotorOffsets(output);

    // Set metadata
    output->fetchTime = TimeManager::getEpochTime();
    output->isValid = true;

    Logger::logf(LOG_INFO, CAT_SYSTEM,
                "NOAA: Successfully fetched %u hours of tide data",
                output->recordCount);

    return SUCCESS;
}

const char* NOAAClient::getErrorMessage(FetchResult result) {
    switch (result) {
        case SUCCESS:
            return "Success";
        case NETWORK_ERROR:
            return "Network error - check WiFi connection";
        case TIMEOUT:
            return "Request timeout - server not responding";
        case INVALID_STATION:
            return "Invalid station ID - check configuration";
        case PARSE_ERROR:
            return "Failed to parse response - NOAA API may have changed";
        case INCOMPLETE_DATA:
            return "Incomplete data - expected 24 hours";
        case NO_TIME_SYNC:
            return "Time not synchronized - sync with NTP first";
        case CONFIG_ERROR:
            return "Configuration error - check station ID";
        default:
            return "Unknown error";
    }
}

String NOAAClient::buildRequestURL(const char* stationID, const char* dateStr) {
    String url = NOAA_API_BASE;
    url += "?product=predictions";
    url += "&application=TideClock";
    url += "&begin_date=" + String(dateStr);
    url += "&end_date=" + String(dateStr);
    url += "&datum=MLLW";
    url += "&station=" + String(stationID);
    url += "&time_zone=lst_ldt";
    url += "&units=english";
    url += "&interval=h";
    url += "&format=json";

    return url;
}

bool NOAAClient::parseJSON(const String& response, TideDataset* output) {
    // Create JSON document (8KB should be sufficient for 24 hours)
    DynamicJsonDocument doc(8192);

    // Parse JSON
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        Logger::logf(LOG_ERROR, CAT_SYSTEM,
                    "NOAA: JSON parse error: %s", error.c_str());
        return false;
    }

    // Check for error response from NOAA
    if (doc.containsKey("error")) {
        JsonObject errorObj = doc["error"];
        const char* errorMsg = errorObj["message"] | "Unknown NOAA error";
        Logger::logf(LOG_ERROR, CAT_SYSTEM, "NOAA API error: %s", errorMsg);
        return false;
    }

    // Get predictions array
    JsonArray predictions = doc["predictions"];
    if (predictions.isNull()) {
        Logger::error(CAT_SYSTEM, "NOAA: 'predictions' array not found");
        return false;
    }

    size_t predictionCount = predictions.size();
    Logger::logf(LOG_INFO, CAT_SYSTEM,
                "NOAA: Found %u predictions", predictionCount);

    if (predictionCount == 0) {
        Logger::error(CAT_SYSTEM, "NOAA: No predictions in response");
        return false;
    }

    // Get configuration for scaling
    const TideClockConfig& config = ConfigManager::getConfig();
    float minTide = config.minTideHeight;
    float maxTide = config.maxTideHeight;

    // Parse each prediction
    uint8_t validCount = 0;
    bool hourSeen[24] = {false};

    for (JsonVariant pred : predictions) {
        JsonObject predObj = pred.as<JsonObject>();

        // Extract timestamp and value
        const char* timestamp = predObj["t"];
        const char* valueStr = predObj["v"];

        if (timestamp == nullptr || valueStr == nullptr) {
            Logger::warning(CAT_SYSTEM, "NOAA: Missing timestamp or value");
            continue;
        }

        // Extract hour from timestamp
        uint8_t hour = extractHour(timestamp);
        if (hour == 255 || hour >= 24) {
            Logger::logf(LOG_WARNING, CAT_SYSTEM,
                        "NOAA: Invalid hour in timestamp: %s", timestamp);
            continue;
        }

        // Check for duplicate hours
        if (hourSeen[hour]) {
            Logger::logf(LOG_WARNING, CAT_SYSTEM,
                        "NOAA: Duplicate hour %u - using first occurrence", hour);
            continue;
        }
        hourSeen[hour] = true;

        // Parse tide height
        float tideHeight = atof(valueStr);

        // Populate hourly data
        HourlyTideData* hourData = &output->hours[hour];
        hourData->hour = hour;

        // Copy timestamp
        strncpy(hourData->timestamp, timestamp, sizeof(hourData->timestamp) - 1);
        hourData->timestamp[sizeof(hourData->timestamp) - 1] = '\0';

        hourData->rawTideHeight = tideHeight;
        hourData->scaledRunTime = scaleToRunTime(tideHeight, minTide, maxTide);
        hourData->finalRunTime = 0;  // Will be set in applyMotorOffsets()

        validCount++;

        Logger::logf(LOG_INFO, CAT_SYSTEM,
                    "NOAA: Hour %02u: %.2f ft -> %u ms",
                    hour, tideHeight, hourData->scaledRunTime);
    }

    output->recordCount = validCount;

    // Extract station name if available
    if (doc.containsKey("metadata")) {
        JsonObject metadata = doc["metadata"];
        const char* name = metadata["name"];
        if (name != nullptr) {
            strncpy(output->stationName, name, sizeof(output->stationName) - 1);
            output->stationName[sizeof(output->stationName) - 1] = '\0';
            Logger::logf(LOG_INFO, CAT_SYSTEM, "NOAA: Station name: %s", name);
        }
    }

    return validCount > 0;
}

bool NOAAClient::validateData(TideDataset* data) {
    if (data == nullptr) {
        return false;
    }

    // Check record count
    if (data->recordCount < 24) {
        Logger::logf(LOG_ERROR, CAT_SYSTEM,
                    "NOAA: Incomplete data - expected 24 hours, got %u",
                    data->recordCount);
        return false;
    }

    // Check that all hours 0-23 have valid data
    for (uint8_t hour = 0; hour < 24; hour++) {
        HourlyTideData* hourData = &data->hours[hour];

        if (hourData->timestamp[0] == '\0') {
            Logger::logf(LOG_ERROR, CAT_SYSTEM,
                        "NOAA: Missing data for hour %u", hour);
            return false;
        }

        // Check for NaN or invalid values
        if (isnan(hourData->rawTideHeight)) {
            Logger::logf(LOG_ERROR, CAT_SYSTEM,
                        "NOAA: Invalid tide height for hour %u", hour);
            return false;
        }
    }

    Logger::info(CAT_SYSTEM, "NOAA: Data validation passed");
    return true;
}

uint16_t NOAAClient::scaleToRunTime(float tideHeight, float minTide, float maxTide) {
    // Handle edge case: zero range
    float tideRange = maxTide - minTide;
    if (tideRange <= 0.0) {
        Logger::warning(CAT_SYSTEM, "NOAA: Invalid tide range - using 0ms");
        return 0;
    }

    // Normalize tide height to 0.0-1.0 range
    float normalized = (tideHeight - minTide) / tideRange;

    // Scale to motor run time (0-9000 ms)
    float scaledTime = normalized * MAX_RUN_TIME_MS;

    // Clamp to valid range
    if (scaledTime < 0.0) {
        Logger::logf(LOG_WARNING, CAT_SYSTEM,
                    "NOAA: Tide %.2f below minimum, clamped to 0ms", tideHeight);
        scaledTime = 0.0;
    } else if (scaledTime > MAX_RUN_TIME_MS) {
        Logger::logf(LOG_WARNING, CAT_SYSTEM,
                    "NOAA: Tide %.2f above maximum, clamped to %ums",
                    tideHeight, MAX_RUN_TIME_MS);
        scaledTime = MAX_RUN_TIME_MS;
    }

    return (uint16_t)(scaledTime + 0.5);  // Round to nearest integer
}

void NOAAClient::applyMotorOffsets(TideDataset* data) {
    if (data == nullptr) {
        return;
    }

    Logger::info(CAT_SYSTEM, "NOAA: Applying motor offsets...");

    for (uint8_t hour = 0; hour < 24; hour++) {
        HourlyTideData* hourData = &data->hours[hour];

        // Get motor offset for this hour (motor index = hour)
        float offset = ConfigManager::getMotorOffset(hour);

        // Apply offset
        float finalTime = hourData->scaledRunTime * offset;

        // Clamp to maximum
        if (finalTime > MAX_RUN_TIME_MS) {
            finalTime = MAX_RUN_TIME_MS;
        }

        // Round and store
        hourData->finalRunTime = (uint16_t)(finalTime + 0.5);

        if (offset != 1.0) {
            Logger::logf(LOG_INFO, CAT_SYSTEM,
                        "NOAA: Hour %02u: offset %.3f -> %u ms -> %u ms",
                        hour, offset, hourData->scaledRunTime, hourData->finalRunTime);
        }
    }
}

uint8_t NOAAClient::extractHour(const char* timestamp) {
    if (timestamp == nullptr) {
        return 255;
    }

    // Expected format: "YYYY-MM-DD HH:MM"
    // Hour starts at position 11
    size_t len = strlen(timestamp);
    if (len < 13) {
        return 255;
    }

    // Extract hour digits
    char hourStr[3];
    hourStr[0] = timestamp[11];
    hourStr[1] = timestamp[12];
    hourStr[2] = '\0';

    int hour = atoi(hourStr);
    if (hour < 0 || hour > 23) {
        return 255;
    }

    return (uint8_t)hour;
}

int NOAAClient::httpGetWithRetry(const String& url, String& response, uint16_t timeoutMs) {
    HTTPClient http;
    int httpCode = -1;

    for (int attempt = 1; attempt <= NOAA_RETRY_ATTEMPTS; attempt++) {
        Logger::logf(LOG_INFO, CAT_SYSTEM,
                    "NOAA: HTTP request attempt %d/%d",
                    attempt, NOAA_RETRY_ATTEMPTS);

        http.begin(url);
        http.setTimeout(timeoutMs);

        httpCode = http.GET();

        if (httpCode == 200) {
            response = http.getString();
            http.end();
            return httpCode;
        }

        http.end();

        // If not last attempt, wait before retry
        if (attempt < NOAA_RETRY_ATTEMPTS) {
            unsigned long retryDelay = NOAA_RETRY_DELAY_MS * (1 << (attempt - 1));  // Exponential backoff
            Logger::logf(LOG_WARNING, CAT_SYSTEM,
                        "NOAA: Request failed (code %d), retrying in %lu ms",
                        httpCode, retryDelay);
            delay(retryDelay);
        }
    }

    Logger::logf(LOG_ERROR, CAT_SYSTEM,
                "NOAA: All %d attempts failed", NOAA_RETRY_ATTEMPTS);
    return httpCode;
}
