/**
 * TideClock Web Server
 *
 * HTTP server with REST API and HTML interface
 * Handles all web-based control and monitoring
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WebServer.h>

class TideClockWebServer {
public:
    /**
     * Initialize web server
     */
    static void begin();

    /**
     * Handle client requests (call from loop)
     */
    static void handle();

    /**
     * Stop web server
     */
    static void stop();

    /**
     * Check if server is running
     */
    static bool isRunning();

private:
    static WebServer* server;
    static bool running;

    // Route handlers
    static void handleRoot();
    static void handleNotFound();

    // API endpoint handlers
    static void handleGetStatus();
    static void handleGetSwitches();
    static void handleGetLogs();
    static void handleHome();
    static void handleEmergencyStop();
    static void handleClearStop();
    static void handleTestMotor();
    static void handleSaveConfig();

    // Phase 3: NOAA Integration endpoints
    static void handleFetchTide();
    static void handleGetTideData();
    static void handleRunTide();
    static void handleSyncTime();

    // Motor offset calibration endpoints
    static void handleGetMotorOffsets();
    static void handleSaveMotorOffsets();
    static void handleResetMotorOffsets();

    // Helper functions
    static void sendJSON(int code, const char* json);
    static void sendError(int code, const char* message);
    static void sendSuccess(const char* message);
};

#endif // WEB_SERVER_H
