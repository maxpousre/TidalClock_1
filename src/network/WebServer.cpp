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
#include "../hardware/LEDController.h"
#include "../utils/Logger.h"
#include "WiFiManager.h"
#include <ArduinoJson.h>
#include <esp_system.h>

// Phase 3: NOAA Integration
#include "TimeManager.h"
#include "NOAAClient.h"
#include "../data/TideData.h"

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

    // Phase 3: NOAA Integration routes
    server->on("/api/fetch", HTTP_POST, handleFetchTide);
    server->on("/api/tide-data", HTTP_GET, handleGetTideData);
    server->on("/api/run-tide", HTTP_POST, handleRunTide);
    server->on("/api/sync-time", HTTP_POST, handleSyncTime);

    // Motor offset calibration routes
    server->on("/api/motor-offsets", HTTP_GET, handleGetMotorOffsets);
    server->on("/api/motor-offsets", HTTP_POST, handleSaveMotorOffsets);
    server->on("/api/reset-offsets", HTTP_POST, handleResetMotorOffsets);

    // Phase 4: LED Control routes
    server->on("/api/led-config", HTTP_GET, handleGetLEDConfig);
    server->on("/api/led-config", HTTP_POST, handleSaveLEDConfig);
    server->on("/api/led-test", HTTP_POST, handleLEDTest);

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
    cfg["stationID"] = config.stationID;
    cfg["minTideHeight"] = config.minTideHeight;
    cfg["maxTideHeight"] = config.maxTideHeight;

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

    // LED status (Phase 4)
    JsonObject led = doc.createNestedObject("led");
    led["enabled"] = LEDController::isEnabled();
    led["status"] = LEDController::getStatusString();
    led["withinActiveHours"] = LEDController::isWithinActiveHours();

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

    // Phase 3: Update NOAA configuration if provided
    if (doc.containsKey("stationID")) {
        const char* stationID = doc["stationID"];
        ConfigManager::setNOAAStation(stationID);
        configChanged = true;
        Logger::logf(LOG_INFO, CAT_SYSTEM, "NOAA station ID updated: %s", stationID);
    }

    if (doc.containsKey("minTide") && doc.containsKey("maxTide")) {
        float minTide = doc["minTide"];
        float maxTide = doc["maxTide"];
        ConfigManager::setTideRange(minTide, maxTide);
        configChanged = true;
        Logger::logf(LOG_INFO, CAT_SYSTEM, "Tide range updated: %.1f - %.1f ft", minTide, maxTide);
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

// ============================================================================
// PHASE 3: NOAA INTEGRATION API HANDLERS
// ============================================================================

void TideClockWebServer::handleFetchTide() {
    Logger::info(CAT_WEB, "API: Fetch tide data requested");

    // Check if time is synced
    if (!TimeManager::isTimeSynced()) {
        sendError(400, "Time not synchronized - sync with NTP first");
        return;
    }

    // Get station ID from config
    const TideClockConfig& config = ConfigManager::getConfig();
    if (strlen(config.stationID) == 0) {
        sendError(400, "NOAA station ID not configured");
        return;
    }

    // Set state to FETCHING_DATA
    StateManager::setState(STATE_FETCHING_DATA);

    // Fetch data from NOAA
    TideDataset* dataset = TideDataManager::getMutableDataset();
    NOAAClient::FetchResult result = NOAAClient::fetchTidePredictions(
        config.stationID,
        dataset,
        10000  // 10 second timeout
    );

    // Return to READY state
    StateManager::setState(STATE_READY);

    // Handle result
    if (result == NOAAClient::SUCCESS) {
        TideDataManager::setData(dataset);

        // Build success response
        StaticJsonDocument<512> doc;
        doc["success"] = true;
        doc["message"] = "Fetched " + String(dataset->recordCount) + " hours of tide data";
        doc["stationID"] = dataset->stationID;
        doc["stationName"] = dataset->stationName;
        doc["recordCount"] = dataset->recordCount;
        doc["fetchTime"] = TimeManager::getFormattedDateTime();

        // Tide range
        float minTide = 999.0;
        float maxTide = -999.0;
        for (uint8_t i = 0; i < 24; i++) {
            float tide = dataset->hours[i].rawTideHeight;
            if (tide < minTide) minTide = tide;
            if (tide > maxTide) maxTide = tide;
        }
        JsonObject tideRange = doc.createNestedObject("tideRange");
        tideRange["min"] = minTide;
        tideRange["max"] = maxTide;

        String output;
        serializeJson(doc, output);
        sendJSON(200, output.c_str());
    } else {
        const char* errorMsg = NOAAClient::getErrorMessage(result);
        TideDataManager::setError(errorMsg);
        sendError(500, errorMsg);
    }
}

void TideClockWebServer::handleGetTideData() {
    const TideDataset* dataset = TideDataManager::getCurrentDataset();

    StaticJsonDocument<4096> doc;  // Larger buffer for 24 hours of data

    if (!TideDataManager::isDataValid()) {
        doc["available"] = false;
        doc["message"] = "No valid tide data - fetch data first";

        String output;
        serializeJson(doc, output);
        sendJSON(200, output.c_str());
        return;
    }

    // Build response with all tide data
    doc["available"] = true;
    doc["stationID"] = dataset->stationID;
    doc["stationName"] = dataset->stationName;
    doc["fetchTime"] = ctime(&dataset->fetchTime);
    doc["dataAge"] = TideDataManager::getDataAgeString();
    doc["isStale"] = TideDataManager::isDataStale();
    doc["recordCount"] = dataset->recordCount;

    // Current hour
    if (TimeManager::isTimeSynced()) {
        doc["currentHour"] = TimeManager::getCurrentHour();
    }

    // Hourly data array
    JsonArray hours = doc.createNestedArray("hours");
    for (uint8_t i = 0; i < 24; i++) {
        const HourlyTideData* hourData = &dataset->hours[i];

        JsonObject hour = hours.createNestedObject();
        hour["hour"] = hourData->hour;
        hour["timestamp"] = hourData->timestamp;
        hour["tideHeight"] = hourData->rawTideHeight;
        hour["scaledTime"] = hourData->scaledRunTime;
        hour["finalTime"] = hourData->finalRunTime;
        hour["offset"] = ConfigManager::getMotorOffset(i);
    }

    String output;
    serializeJson(doc, output);
    sendJSON(200, output.c_str());
}

void TideClockWebServer::handleRunTide() {
    Logger::info(CAT_WEB, "API: Run tide sequence requested");

    // Parse request body for dry run option
    bool dryRun = false;
    if (server->hasArg("plain")) {
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, server->arg("plain"));

        if (!error) {
            dryRun = doc["dryRun"] | false;
        }
    }

    // Get tide data
    TideDataset* dataset = TideDataManager::getMutableDataset();
    if (!TideDataManager::isDataValid()) {
        sendError(400, "No valid tide data - fetch data first");
        return;
    }

    // Check system state
    if (StateManager::getState() != STATE_READY) {
        String errorMsg = String("System not ready - current state: ") + StateManager::getStateName();
        sendError(400, errorMsg.c_str());
        return;
    }

    // Set state
    if (!dryRun) {
        StateManager::setState(STATE_RUNNING_TIDE);
    }

    // Run tide sequence
    bool success = MotorController::runTideSequence(dataset, dryRun);

    // Return to READY state
    if (!dryRun) {
        StateManager::setState(STATE_READY);
    }

    // Send response
    if (success) {
        if (dryRun) {
            sendSuccess("Dry run completed - check logs for details");
        } else {
            sendSuccess("Tide sequence completed - 24 motors positioned");
        }
    } else {
        sendError(500, "Tide sequence failed - check logs for details");
    }
}

void TideClockWebServer::handleSyncTime() {
    Logger::info(CAT_WEB, "API: NTP sync requested");

    bool success = TimeManager::syncWithNTP(10000);

    if (success) {
        StaticJsonDocument<256> doc;
        doc["success"] = true;
        doc["message"] = "Time synchronized successfully";
        doc["currentTime"] = TimeManager::getFormattedDateTime();
        doc["epochTime"] = TimeManager::getEpochTime();

        String output;
        serializeJson(doc, output);
        sendJSON(200, output.c_str());
    } else {
        sendError(500, "NTP sync failed - check WiFi connection");
    }
}

void TideClockWebServer::handleGetMotorOffsets() {
    Logger::info(CAT_WEB, "API: Get motor offsets requested");

    StaticJsonDocument<1024> doc;
    doc["success"] = true;

    // Create array of all 24 motor offsets
    JsonArray offsets = doc.createNestedArray("offsets");
    for (uint8_t i = 0; i < 24; i++) {
        offsets.add(ConfigManager::getMotorOffset(i));
    }

    String output;
    serializeJson(doc, output);
    sendJSON(200, output.c_str());
}

void TideClockWebServer::handleSaveMotorOffsets() {
    Logger::info(CAT_WEB, "API: Save motor offsets requested");

    // Parse request body
    if (!server->hasArg("plain")) {
        sendError(400, "Missing request body");
        return;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, server->arg("plain"));

    if (error) {
        sendError(400, "Invalid JSON");
        return;
    }

    // Validate that offsets array exists
    if (!doc.containsKey("offsets")) {
        sendError(400, "Missing 'offsets' array");
        return;
    }

    JsonArray offsets = doc["offsets"];
    if (offsets.size() != 24) {
        sendError(400, "Expected 24 motor offsets");
        return;
    }

    // Validate and save each offset
    uint8_t successCount = 0;
    for (uint8_t i = 0; i < 24; i++) {
        float offset = offsets[i];

        // Validate range (0.8 to 1.2)
        if (offset < 0.8 || offset > 1.2) {
            Logger::logf(LOG_WARNING, CAT_WEB,
                        "Motor %u offset %.3f out of range (0.8-1.2)", i, offset);
            continue;
        }

        // Save to config
        ConfigManager::setMotorOffset(i, offset);
        successCount++;
    }

    if (successCount == 24) {
        // Save to EEPROM
        if (ConfigManager::save()) {
            Logger::info(CAT_WEB, "All motor offsets saved to EEPROM");
            sendSuccess("Motor offsets saved successfully");
        } else {
            sendError(500, "Failed to save configuration to EEPROM");
        }
    } else {
        String errorMsg = String("Only ") + successCount + "/24 offsets were valid";
        sendError(400, errorMsg.c_str());
    }
}

void TideClockWebServer::handleResetMotorOffsets() {
    Logger::info(CAT_WEB, "API: Reset motor offsets requested");

    // Reset all offsets to 1.0
    ConfigManager::resetMotorOffsets();

    // Save to EEPROM
    if (ConfigManager::save()) {
        Logger::info(CAT_WEB, "Motor offsets reset to 1.0 and saved");
        sendSuccess("All motor offsets reset to 1.0");
    } else {
        sendError(500, "Failed to save configuration to EEPROM");
    }
}

// ============================================================================
// LED CONTROL ENDPOINTS (Phase 4)
// ============================================================================

void TideClockWebServer::handleGetLEDConfig() {
    Logger::info(CAT_WEB, "API: Get LED configuration");

    const TideClockConfig& config = ConfigManager::getConfig();

    StaticJsonDocument<512> doc;
    doc["enabled"] = config.ledEnabled;
    doc["pin"] = config.ledPin;
    doc["count"] = config.ledCount;
    doc["mode"] = config.ledMode;
    doc["brightness"] = config.ledBrightness;
    doc["colorIndex"] = config.ledColorIndex;
    doc["startHour"] = config.ledStartHour;
    doc["endHour"] = config.ledEndHour;

    // Add predefined color names for UI
    JsonArray colors = doc.createNestedArray("colorNames");
    colors.add("Warm White");
    colors.add("Cool White");
    colors.add("Red");
    colors.add("Orange");
    colors.add("Yellow");
    colors.add("Green");
    colors.add("Cyan");
    colors.add("Blue");
    colors.add("Purple");
    colors.add("Magenta");
    colors.add("Ocean Blue");
    colors.add("Deep Teal");

    String output;
    serializeJson(doc, output);
    sendJSON(200, output.c_str());
}

void TideClockWebServer::handleSaveLEDConfig() {
    Logger::info(CAT_WEB, "API: Save LED configuration");

    // Parse JSON request body
    if (!server->hasArg("plain")) {
        sendError(400, "Missing request body");
        return;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, server->arg("plain"));

    if (error) {
        Logger::logf(LOG_ERROR, CAT_WEB, "JSON parse error: %s", error.c_str());
        sendError(400, "Invalid JSON");
        return;
    }

    // Update LED configuration with validation
    bool configChanged = false;

    if (doc.containsKey("enabled")) {
        ConfigManager::setLEDEnabled(doc["enabled"]);
        LEDController::setEnabled(doc["enabled"]);
        configChanged = true;
    }

    if (doc.containsKey("pin")) {
        uint8_t pin = doc["pin"];
        ConfigManager::setLEDPin(pin);
        configChanged = true;
        // Note: Pin change requires reinit, handled after all changes
    }

    if (doc.containsKey("count")) {
        uint16_t count = doc["count"];
        ConfigManager::setLEDCount(count);
        configChanged = true;
        // Note: Count change requires reinit, handled after all changes
    }

    if (doc.containsKey("mode")) {
        uint8_t mode = doc["mode"];
        ConfigManager::setLEDMode(mode);
        LEDController::setMode(mode);
        configChanged = true;
    }

    if (doc.containsKey("brightness")) {
        uint8_t brightness = doc["brightness"];
        ConfigManager::setLEDBrightness(brightness);
        LEDController::setBrightness(brightness);
        configChanged = true;
    }

    if (doc.containsKey("colorIndex")) {
        uint8_t colorIndex = doc["colorIndex"];
        ConfigManager::setLEDColorIndex(colorIndex);
        LEDController::setColorIndex(colorIndex);
        configChanged = true;
    }

    if (doc.containsKey("startHour") && doc.containsKey("endHour")) {
        uint8_t startHour = doc["startHour"];
        uint8_t endHour = doc["endHour"];
        ConfigManager::setLEDActiveHours(startHour, endHour);
        LEDController::setActiveHours(startHour, endHour);
        configChanged = true;
    }

    // If pin or count changed, reinitialize LED controller
    if (doc.containsKey("pin") || doc.containsKey("count")) {
        const TideClockConfig& config = ConfigManager::getConfig();
        if (!LEDController::reinit(config.ledPin, config.ledCount)) {
            sendError(500, "Failed to reinitialize LED controller");
            return;
        }
    }

    // Save configuration to EEPROM
    if (configChanged) {
        if (ConfigManager::save()) {
            Logger::info(CAT_WEB, "LED configuration saved");
            sendSuccess("LED configuration saved successfully");
        } else {
            sendError(500, "Failed to save configuration to EEPROM");
        }
    } else {
        sendSuccess("No changes to save");
    }
}

void TideClockWebServer::handleLEDTest() {
    Logger::info(CAT_WEB, "API: LED test pattern requested");

    // Trigger test pattern mode
    LEDController::runTestPattern();

    sendSuccess("Test pattern activated");
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
