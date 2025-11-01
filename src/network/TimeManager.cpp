/**
 * TideClock Time Manager Implementation
 */

#include "TimeManager.h"
#include "../utils/Logger.h"
#include <WiFi.h>

// NTP Configuration
#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.nist.gov"
#define NTP_SERVER3 "time.google.com"

// Static member initialization
bool TimeManager::timeSynced = false;
char TimeManager::timezoneStr[48] = "UTC0";

void TimeManager::initialize(const char* timezone) {
    Logger::info(CAT_SYSTEM, "Initializing Time Manager...");

    if (timezone != nullptr && strlen(timezone) > 0) {
        strncpy(timezoneStr, timezone, sizeof(timezoneStr) - 1);
        timezoneStr[sizeof(timezoneStr) - 1] = '\0';
    }

    // Configure timezone
    setenv("TZ", timezoneStr, 1);
    tzset();

    Logger::logf(LOG_INFO, CAT_SYSTEM, "Timezone set to: %s", timezoneStr);
}

bool TimeManager::syncWithNTP(uint16_t timeoutMs) {
    Logger::info(CAT_SYSTEM, "Synchronizing with NTP servers...");

    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Logger::warning(CAT_SYSTEM, "Cannot sync NTP - WiFi not connected");
        return false;
    }

    // Configure NTP (using SNTP)
    configTime(0, 0, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);

    // Wait for time to be set
    unsigned long startTime = millis();
    struct tm timeinfo;

    while (millis() - startTime < timeoutMs) {
        if (getLocalTime(&timeinfo)) {
            // Check if year is reasonable (> 2020)
            if (timeinfo.tm_year + 1900 > 2020) {
                timeSynced = true;
                Logger::logf(LOG_INFO, CAT_SYSTEM,
                           "NTP sync successful: %04d-%02d-%02d %02d:%02d:%02d",
                           timeinfo.tm_year + 1900,
                           timeinfo.tm_mon + 1,
                           timeinfo.tm_mday,
                           timeinfo.tm_hour,
                           timeinfo.tm_min,
                           timeinfo.tm_sec);
                return true;
            }
        }
        delay(100);
    }

    Logger::error(CAT_SYSTEM, "NTP sync timeout");
    return false;
}

bool TimeManager::isTimeSynced() {
    // Double-check by verifying current year
    if (timeSynced) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            return (timeinfo.tm_year + 1900 > 2020);
        }
    }
    return false;
}

void TimeManager::getCurrentDateTime(struct tm* timeinfo) {
    if (timeinfo == nullptr) {
        return;
    }

    if (!getLocalTime(timeinfo)) {
        Logger::warning(CAT_SYSTEM, "Failed to get local time");
        memset(timeinfo, 0, sizeof(struct tm));
    }
}

String TimeManager::getFormattedDate() {
    struct tm timeinfo;
    getCurrentDateTime(&timeinfo);

    char buffer[16];
    sprintf(buffer, "%04d%02d%02d",
            timeinfo.tm_year + 1900,
            timeinfo.tm_mon + 1,
            timeinfo.tm_mday);

    return String(buffer);
}

String TimeManager::getFormattedDateTime() {
    struct tm timeinfo;
    getCurrentDateTime(&timeinfo);

    char buffer[32];
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
            timeinfo.tm_year + 1900,
            timeinfo.tm_mon + 1,
            timeinfo.tm_mday,
            timeinfo.tm_hour,
            timeinfo.tm_min,
            timeinfo.tm_sec);

    return String(buffer);
}

uint8_t TimeManager::getCurrentHour() {
    struct tm timeinfo;
    getCurrentDateTime(&timeinfo);
    return timeinfo.tm_hour;
}

uint8_t TimeManager::getCurrentDay() {
    struct tm timeinfo;
    getCurrentDateTime(&timeinfo);
    return timeinfo.tm_mday;
}

time_t TimeManager::getEpochTime() {
    return time(nullptr);
}

String TimeManager::getAgeString(uint32_t seconds) {
    if (seconds < 60) {
        return String(seconds) + " seconds ago";
    } else if (seconds < 3600) {
        uint32_t minutes = seconds / 60;
        return String(minutes) + (minutes == 1 ? " minute ago" : " minutes ago");
    } else if (seconds < 86400) {
        uint32_t hours = seconds / 3600;
        return String(hours) + (hours == 1 ? " hour ago" : " hours ago");
    } else {
        uint32_t days = seconds / 86400;
        return String(days) + (days == 1 ? " day ago" : " days ago");
    }
}
