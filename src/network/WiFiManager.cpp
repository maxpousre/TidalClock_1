/**
 * TideClock WiFi Manager Implementation
 */

#include "WiFiManager.h"
#include "../config.h"
#include "../core/ConfigManager.h"
#include "../utils/Logger.h"
#include <WiFi.h>

// Static member initialization
WiFiMode WiFiManager::currentMode = WIFI_MODE_DISCONNECTED;
unsigned long WiFiManager::lastConnectionAttempt = 0;
uint8_t WiFiManager::connectionAttempts = 0;

void WiFiManager::begin() {
    Logger::info(CAT_SYSTEM, "Initializing WiFi Manager...");

    // Set WiFi mode
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    currentMode = WIFI_MODE_DISCONNECTED;
    connectionAttempts = 0;
}

bool WiFiManager::connect() {
    const TideClockConfig& config = ConfigManager::getConfig();

    // Check if we have WiFi credentials
    if (strlen(config.wifiSSID) == 0) {
        Logger::warning(CAT_SYSTEM, "No WiFi credentials configured");
        Logger::info(CAT_SYSTEM, "Starting AP mode for initial setup...");
        startAPMode();
        return false;
    }

    Logger::logf(LOG_INFO, CAT_SYSTEM,
                 "Attempting to connect to WiFi: %s", config.wifiSSID);

    // Try to connect with retries
    for (uint8_t attempt = 1; attempt <= WIFI_MAX_RETRIES; attempt++) {
        Logger::logf(LOG_INFO, CAT_SYSTEM,
                     "Connection attempt %d/%d...", attempt, WIFI_MAX_RETRIES);

        if (tryStationMode(config.wifiSSID, config.wifiPassword)) {
            currentMode = WIFI_MODE_STATION;
            connectionAttempts = 0;

            Logger::info(CAT_SYSTEM, "WiFi connected successfully!");
            Logger::logf(LOG_INFO, CAT_SYSTEM, "IP Address: %s", WiFi.localIP().toString().c_str());
            Logger::logf(LOG_INFO, CAT_SYSTEM, "Signal Strength: %d dBm", WiFi.RSSI());

            return true;
        }

        delay(1000);  // Brief delay between attempts
    }

    // All attempts failed - switch to AP mode
    Logger::error(CAT_SYSTEM, "Failed to connect after 3 attempts");
    Logger::info(CAT_SYSTEM, "Starting AP mode...");
    startAPMode();

    return false;
}

bool WiFiManager::tryStationMode(const char* ssid, const char* password) {
    WiFi.begin(ssid, password);

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > WIFI_CONNECT_TIMEOUT) {
            Logger::warning(CAT_SYSTEM, "Connection timeout");
            WiFi.disconnect();
            return false;
        }
        delay(100);
    }

    return true;
}

void WiFiManager::startAPMode() {
    Logger::info(CAT_SYSTEM, "Starting Access Point mode...");

    // Disconnect from any existing connection
    WiFi.disconnect();

    // Configure AP
    WiFi.mode(WIFI_AP);

    bool success;
    if (strlen(AP_PASSWORD) > 0) {
        success = WiFi.softAP(AP_SSID, AP_PASSWORD);
    } else {
        success = WiFi.softAP(AP_SSID);  // Open network
    }

    if (success) {
        currentMode = WIFI_MODE_AP;

        Logger::separator();
        Logger::info(CAT_SYSTEM, "*** ACCESS POINT MODE ACTIVE ***");
        Logger::logf(LOG_INFO, CAT_SYSTEM, "SSID: %s", AP_SSID);
        Logger::logf(LOG_INFO, CAT_SYSTEM, "Password: %s",
                     strlen(AP_PASSWORD) > 0 ? AP_PASSWORD : "(Open Network)");
        Logger::logf(LOG_INFO, CAT_SYSTEM, "IP Address: %s",
                     WiFi.softAPIP().toString().c_str());
        Logger::separator();
        Logger::info(CAT_SYSTEM, "Connect to TideClock network and navigate to:");
        Logger::logf(LOG_INFO, CAT_SYSTEM, "http://%s", WiFi.softAPIP().toString().c_str());
        Logger::separator();
    } else {
        Logger::error(CAT_SYSTEM, "Failed to start Access Point!");
        currentMode = WIFI_MODE_DISCONNECTED;
    }
}

void WiFiManager::disconnect() {
    Logger::info(CAT_SYSTEM, "Disconnecting WiFi...");
    WiFi.disconnect();
    currentMode = WIFI_MODE_DISCONNECTED;
}

bool WiFiManager::isConnected() {
    if (currentMode == WIFI_MODE_STATION) {
        return (WiFi.status() == WL_CONNECTED);
    } else if (currentMode == WIFI_MODE_AP) {
        return true;  // AP mode is always "connected"
    }
    return false;
}

WiFiMode WiFiManager::getMode() {
    return currentMode;
}

String WiFiManager::getSSID() {
    if (currentMode == WIFI_MODE_STATION) {
        return WiFi.SSID();
    } else if (currentMode == WIFI_MODE_AP) {
        return String(AP_SSID);
    }
    return "Not Connected";
}

String WiFiManager::getIPAddress() {
    if (currentMode == WIFI_MODE_STATION) {
        return WiFi.localIP().toString();
    } else if (currentMode == WIFI_MODE_AP) {
        return WiFi.softAPIP().toString();
    }
    return "0.0.0.0";
}

int WiFiManager::getSignalStrength() {
    if (currentMode == WIFI_MODE_STATION && WiFi.status() == WL_CONNECTED) {
        return WiFi.RSSI();
    }
    return 0;
}

const char* WiFiManager::getModeName() {
    switch (currentMode) {
        case WIFI_MODE_STATION:      return "Station";
        case WIFI_MODE_AP:           return "Access Point";
        case WIFI_MODE_DISCONNECTED: return "Disconnected";
        default:                     return "Unknown";
    }
}

void WiFiManager::handle() {
    // Check if Station mode connection was lost
    if (currentMode == WIFI_MODE_STATION && WiFi.status() != WL_CONNECTED) {
        Logger::warning(CAT_SYSTEM, "WiFi connection lost - attempting reconnect...");
        currentMode = WIFI_MODE_DISCONNECTED;

        // Auto-reconnect will be handled by WiFi library
        // If that fails, user can manually reconnect via web UI (if in AP mode)
    }
}

void WiFiManager::printStatus() {
    Logger::separator();
    Serial.println("WIFI STATUS:");
    Logger::separator();
    Serial.printf("Mode:             %s\n", getModeName());
    Serial.printf("SSID:             %s\n", getSSID().c_str());
    Serial.printf("IP Address:       %s\n", getIPAddress().c_str());

    if (currentMode == WIFI_MODE_STATION) {
        Serial.printf("Signal Strength:  %d dBm\n", getSignalStrength());
        Serial.printf("MAC Address:      %s\n", WiFi.macAddress().c_str());
    } else if (currentMode == WIFI_MODE_AP) {
        Serial.printf("Clients:          %d\n", WiFi.softAPgetStationNum());
    }

    Logger::separator();
}
