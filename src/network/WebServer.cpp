/**
 * TideClock Web Server Implementation
 */

#include "WebServer.h"
#include "WebUI.h"
#include "../config.h"
#include "../core/StateManager.h"
#include "../core/ConfigManager.h"
#include "../hardware/MotorController.h"
#include "../hardware/SwitchReader.h"
#include "../utils/Logger.h"
#include "WiFiManager.h"
#include <ArduinoJson.h>
#include <esp_system.h>

// Static member initialization
WebServer* TideClockWebServer::server = nullptr;
bool TideClockWebServer::running = false;

void TideClockWebServer::begin() {
    Logger::info(CAT_SYSTEM, "Starting web server...");

    if (server != nullptr) {
        delete server;
    }

    server = new WebServer(WEB_SERVER_PORT);

    // Register route handlers
    server->on("/", HTTP_GET, handleRoot);
    server->on("/api/status", HTTP_GET, handleGetStatus);
    server->on("/api/switches", HTTP_GET, handleGetSwitches);
    server->on("/api/logs", HTTP_GET, handleGetLogs);
    server->on("/api/home", HTTP_POST, handleHome);
    server->on("/api/emergency-stop", HTTP_POST, handleEmergencyStop);
    server->on("/api/clear-stop", HTTP_POST, handleClearStop);
    server->on("/api/test-motor", HTTP_POST, handleTestMotor);
    server->on("/api/save-config", HTTP_POST, handleSaveConfig);
    server->on("/api/fetch", HTTP_POST, handleFetchStub);
    server->onNotFound(handleNotFound);

    server->begin();
    running = true;

    Logger::logf(LOG_INFO, CAT_SYSTEM,
                 "Web server started on port %d", WEB_SERVER_PORT);
}

void TideClockWebServer::handle() {
    if (server != nullptr && running) {
        server->handleClient();
    }
}

void TideClockWebServer::stop() {
    if (server != nullptr) {
        server->stop();
        running = false;
        Logger::info(CAT_SYSTEM, "Web server stopped");
    }
}

bool TideClockWebServer::isRunning() {
    return running;
}

// ============================================================================
// ROUTE HANDLERS
// ============================================================================

void TideClockWebServer::handleRoot() {
    server->send(200, "text/html", getWebUI());
}

void TideClockWebServer::handleNotFound() {
    String message = "404: Not Found\n\n";
    message += "URI: " + server->uri() + "\n";
    message += "Method: " + String((server->method() == HTTP_GET) ? "GET" : "POST");
    server->send(404, "text/plain", message);
}

// ============================================================================
// API ENDPOINT HANDLERS
// ============================================================================

void TideClockWebServer::handleGetStatus() {
    StaticJsonDocument<1024> doc;

    // System state
    doc["state"] = StateManager::getStateName();
    doc["uptime"] = millis() / 1000;  // seconds
    doc["freeHeap"] = esp_get_free_heap_size();

    // WiFi info
    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["mode"] = WiFiManager::getModeName();
    wifi["ssid"] = WiFiManager::getSSID();
    wifi["ip"] = WiFiManager::getIPAddress();
    wifi["rssi"] = WiFiManager::getSignalStrength();
    wifi["connected"] = WiFiManager::isConnected();

    // Configuration
    const TideClockConfig& config = ConfigManager::getConfig();
    JsonObject cfg = doc.createNestedObject("config");
    cfg["switchRelease"] = config.switchReleaseTime;
    cfg["maxRunTime"] = config.maxRunTime;

    // Error message (if in ERROR state)
    if (StateManager::getState() == STATE_ERROR) {
        doc["errorMessage"] = StateManager::getErrorMessage();
    }

    // Motor status
    JsonObject motor = doc.createNestedObject("motor");
    motor["emergencyStop"] = MotorController::isEmergencyStopped();

    // Tide data status (Phase 3)
    JsonObject tideData = doc.createNestedObject("tideData");
    tideData["available"] = false;
    tideData["message"] = "NOAA integration coming in Phase 3";

    // Serialize and send
    String output;
    serializeJson(doc, output);
    sendJSON(200, output.c_str());
}

void TideClockWebServer::handleGetSwitches() {
    StaticJsonDocument<512> doc;
    JsonArray switches = doc.createNestedArray("switches");

    for (int i = 0; i < NUM_MOTORS; i++) {
        JsonObject sw = switches.createNestedObject();
        sw["id"] = i;
        sw["triggered"] = SwitchReader::isSwitchTriggered(i);
    }

    String output;
    serializeJson(doc, output);
    sendJSON(200, output.c_str());
}

void TideClockWebServer::handleGetLogs() {
    StaticJsonDocument<2048> doc;
    JsonArray logs = doc.createNestedArray("logs");

    // Get recent log entries from Logger
    // For now, return a placeholder - Logger will be extended in Phase 2
    JsonObject log = logs.createNestedObject();
    log["timestamp"] = millis();
    log["level"] = "INFO";
    log["category"] = "SYSTEM";
    log["message"] = "Web interface active";

    String output;
    serializeJson(doc, output);
    sendJSON(200, output.c_str());
}

void TideClockWebServer::handleHome() {
    // Check if homing is allowed
    if (!StateManager::canHome()) {
        sendError(400, "Cannot home motors in current state");
        return;
    }

    // Set state to HOMING
    StateManager::setState(STATE_HOMING);
    Logger::info(CAT_SYSTEM, "Homing initiated via web interface");

    // Start homing in background (non-blocking for web response)
    // We'll return immediately and let the operation complete
    sendSuccess("Homing sequence started");

    // Perform homing
    uint8_t homedCount = MotorController::homeAllMotors();

    // Return to READY state
    StateManager::setState(STATE_READY);

    Logger::logf(LOG_INFO, CAT_SYSTEM,
                 "Homing complete: %d/%d motors homed", homedCount, NUM_MOTORS);
}

void TideClockWebServer::handleEmergencyStop() {
    Logger::warning(CAT_SYSTEM, "Emergency stop triggered via web interface");

    MotorController::emergencyStopAll();
    StateManager::enterEmergencyStop();

    sendSuccess("Emergency stop activated");
}

void TideClockWebServer::handleClearStop() {
    if (StateManager::getState() != STATE_EMERGENCY_STOP) {
        sendError(400, "Emergency stop not active");
        return;
    }

    Logger::info(CAT_SYSTEM, "Clearing emergency stop via web interface");

    MotorController::clearEmergencyStop();
    StateManager::clearEmergencyStop();

    sendSuccess("Emergency stop cleared");
}

void TideClockWebServer::handleTestMotor() {
    // Parse request body
    if (!server->hasArg("plain")) {
        sendError(400, "Missing request body");
        return;
    }

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, server->arg("plain"));

    if (error) {
        sendError(400, "Invalid JSON");
        return;
    }

    // Extract parameters
    if (!doc.containsKey("motor") || !doc.containsKey("action")) {
        sendError(400, "Missing required fields: motor, action");
        return;
    }

    int motor = doc["motor"];
    String action = doc["action"].as<String>();

    // Validate motor index
    if (motor < 0 || motor >= NUM_MOTORS) {
        sendError(400, "Invalid motor index");
        return;
    }

    // Check if testing is allowed
    if (!StateManager::canTest()) {
        sendError(400, "Cannot test motors in current state");
        return;
    }

    // Set state to TESTING
    StateManager::setState(STATE_TESTING);

    // Perform action
    if (action == "forward") {
        int duration = doc["duration"] | 1000;  // Default 1000ms
        if (duration < 0 || duration > MAX_RUN_TIME_MS) {
            StateManager::setState(STATE_READY);
            sendError(400, "Invalid duration (0-9000ms)");
            return;
        }

        Logger::logf(LOG_INFO, CAT_TEST,
                     "Testing motor %d forward for %dms", motor, duration);
        MotorController::runMotorForward(motor, duration);

        sendSuccess("Motor test complete");

    } else if (action == "reverse") {
        int duration = doc["duration"] | 1000;
        if (duration < 0 || duration > MAX_RUN_TIME_MS) {
            StateManager::setState(STATE_READY);
            sendError(400, "Invalid duration (0-9000ms)");
            return;
        }

        Logger::logf(LOG_INFO, CAT_TEST,
                     "Testing motor %d reverse for %dms", motor, duration);
        MotorController::runMotorReverse(motor, duration);

        sendSuccess("Motor test complete");

    } else if (action == "stop") {
        Logger::logf(LOG_INFO, CAT_TEST, "Stopping motor %d", motor);
        MotorController::stopMotor(motor);

        sendSuccess("Motor stopped");

    } else {
        StateManager::setState(STATE_READY);
        sendError(400, "Invalid action (forward/reverse/stop)");
        return;
    }

    // Return to READY state
    StateManager::setState(STATE_READY);
}

void TideClockWebServer::handleSaveConfig() {
    // Parse request body
    if (!server->hasArg("plain")) {
        sendError(400, "Missing request body");
        return;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, server->arg("plain"));

    if (error) {
        sendError(400, "Invalid JSON");
        return;
    }

    // Check if config changes are allowed
    if (!StateManager::canChangeConfig()) {
        sendError(400, "Cannot change config in current state");
        return;
    }

    bool configChanged = false;

    // Update WiFi credentials if provided
    if (doc.containsKey("wifiSSID") && doc.containsKey("wifiPassword")) {
        const char* ssid = doc["wifiSSID"];
        const char* password = doc["wifiPassword"];

        ConfigManager::setWiFiCredentials(ssid, password);
        configChanged = true;

        Logger::info(CAT_SYSTEM, "WiFi credentials updated");
    }

    // Update motor timing if provided
    if (doc.containsKey("switchRelease") && doc.containsKey("maxRunTime")) {
        uint16_t switchRelease = doc["switchRelease"];
        uint16_t maxRunTime = doc["maxRunTime"];

        ConfigManager::setMotorTiming(switchRelease, maxRunTime);
        configChanged = true;

        Logger::info(CAT_SYSTEM, "Motor timing updated");
    }

    // Save to EEPROM
    if (configChanged) {
        if (ConfigManager::save()) {
            sendSuccess("Configuration saved - Restart to apply WiFi changes");
        } else {
            sendError(500, "Failed to save configuration");
        }
    } else {
        sendError(400, "No configuration changes provided");
    }
}

void TideClockWebServer::handleFetchStub() {
    // Phase 3 stub - NOAA integration not yet implemented
    sendError(501, "NOAA integration coming in Phase 3");
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void TideClockWebServer::sendJSON(int code, const char* json) {
    server->send(code, "application/json", json);
}

void TideClockWebServer::sendError(int code, const char* message) {
    StaticJsonDocument<128> doc;
    doc["success"] = false;
    doc["error"] = message;

    String output;
    serializeJson(doc, output);
    sendJSON(code, output.c_str());
}

void TideClockWebServer::sendSuccess(const char* message) {
    StaticJsonDocument<128> doc;
    doc["success"] = true;
    doc["message"] = message;

    String output;
    serializeJson(doc, output);
    sendJSON(200, output.c_str());
}
