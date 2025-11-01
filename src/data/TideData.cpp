/**
 * TideClock Tide Data Manager Implementation
 */

#include "TideData.h"
#include "../network/TimeManager.h"
#include "../utils/Logger.h"
#include <string.h>

// Static member initialization
TideDataset TideDataManager::currentData;

void TideDataManager::clear() {
    Logger::info(CAT_SYSTEM, "Clearing tide data");

    memset(&currentData, 0, sizeof(TideDataset));
    currentData.isValid = false;
    currentData.recordCount = 0;
    currentData.fetchTime = 0;
    currentData.stationID[0] = '\0';
    currentData.stationName[0] = '\0';
    currentData.errorMessage[0] = '\0';

    // Initialize all hourly data
    for (uint8_t i = 0; i < 24; i++) {
        currentData.hours[i].hour = i;
        currentData.hours[i].timestamp[0] = '\0';
        currentData.hours[i].rawTideHeight = 0.0;
        currentData.hours[i].scaledRunTime = 0;
        currentData.hours[i].finalRunTime = 0;
    }
}

bool TideDataManager::isDataValid() {
    return currentData.isValid && currentData.recordCount == 24;
}

bool TideDataManager::isDataStale(uint32_t maxAgeSeconds) {
    if (!currentData.isValid) {
        return true;  // Invalid data is considered stale
    }

    uint32_t ageSeconds = getDataAgeSeconds();
    return ageSeconds > maxAgeSeconds;
}

HourlyTideData* TideDataManager::getHourData(uint8_t hour) {
    if (hour >= 24 || !currentData.isValid) {
        return nullptr;
    }
    return &currentData.hours[hour];
}

const TideDataset* TideDataManager::getCurrentDataset() {
    return &currentData;
}

TideDataset* TideDataManager::getMutableDataset() {
    return &currentData;
}

void TideDataManager::setData(const TideDataset* newData) {
    if (newData == nullptr) {
        Logger::error(CAT_SYSTEM, "Cannot set null tide data");
        return;
    }

    // Copy data
    memcpy(&currentData, newData, sizeof(TideDataset));

    Logger::logf(LOG_INFO, CAT_SYSTEM,
                "Tide data updated: %u records from station %s",
                currentData.recordCount, currentData.stationID);
}

uint32_t TideDataManager::getDataAgeSeconds() {
    if (!currentData.isValid || currentData.fetchTime == 0) {
        return 0;
    }

    time_t currentTime = TimeManager::getEpochTime();
    if (currentTime < currentData.fetchTime) {
        return 0;  // Time error
    }

    return (uint32_t)(currentTime - currentData.fetchTime);
}

String TideDataManager::getDataAgeString() {
    uint32_t ageSeconds = getDataAgeSeconds();
    if (ageSeconds == 0) {
        return "Never";
    }

    return TimeManager::getAgeString(ageSeconds);
}

void TideDataManager::setError(const char* errorMsg) {
    if (errorMsg == nullptr) {
        currentData.errorMessage[0] = '\0';
        return;
    }

    strncpy(currentData.errorMessage, errorMsg, sizeof(currentData.errorMessage) - 1);
    currentData.errorMessage[sizeof(currentData.errorMessage) - 1] = '\0';

    Logger::logf(LOG_ERROR, CAT_SYSTEM, "Tide data error: %s", errorMsg);
}

const char* TideDataManager::getLastError() {
    return currentData.errorMessage;
}
