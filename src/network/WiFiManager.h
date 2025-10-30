/**
 * TideClock WiFi Manager
 *
 * Manages WiFi connectivity with automatic Station/AP fallback
 * - Station Mode: Connect to saved WiFi credentials
 * - AP Mode: Fallback access point for initial setup
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

enum WiFiMode {
    WIFI_MODE_STATION,      // Connected to user's WiFi network
    WIFI_MODE_AP,           // Access Point mode (fallback)
    WIFI_MODE_DISCONNECTED  // Not connected
};

class WiFiManager {
public:
    /**
     * Initialize WiFi subsystem
     */
    static void begin();

    /**
     * Connect to WiFi using credentials from ConfigManager
     * Returns true if connection successful
     */
    static bool connect();

    /**
     * Start Access Point mode (fallback)
     */
    static void startAPMode();

    /**
     * Disconnect from WiFi
     */
    static void disconnect();

    /**
     * Check if connected
     */
    static bool isConnected();

    /**
     * Get current WiFi mode
     */
    static WiFiMode getMode();

    /**
     * Get connection info
     */
    static String getSSID();
    static String getIPAddress();
    static int getSignalStrength();  // RSSI in dBm
    static const char* getModeName();

    /**
     * Handle WiFi events (call from loop)
     */
    static void handle();

    /**
     * Print WiFi status to serial
     */
    static void printStatus();

private:
    static WiFiMode currentMode;
    static unsigned long lastConnectionAttempt;
    static uint8_t connectionAttempts;

    static bool tryStationMode(const char* ssid, const char* password);
};

#endif // WIFI_MANAGER_H
