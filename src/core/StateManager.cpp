/**
 * TideClock State Manager Implementation
 */

#include "StateManager.h"
#include "../utils/Logger.h"

// Static member initialization
SystemState StateManager::currentState = STATE_BOOT;
unsigned long StateManager::stateTimestamp = 0;
char StateManager::errorMessage[128] = "";

void StateManager::begin() {
    currentState = STATE_BOOT;
    stateTimestamp = millis();
    errorMessage[0] = '\0';
    Logger::info(CAT_SYSTEM, "State Manager initialized");
}

SystemState StateManager::getState() {
    return currentState;
}

void StateManager::setState(SystemState newState) {
    if (currentState != newState) {
        SystemState oldState = currentState;
        currentState = newState;
        stateTimestamp = millis();

        Logger::logf(LOG_INFO, CAT_SYSTEM, "State change: %s -> %s",
                     getStateName(oldState), getStateName(newState));

        // Clear error message when leaving ERROR state
        if (oldState == STATE_ERROR && newState != STATE_ERROR) {
            errorMessage[0] = '\0';
        }
    }
}

const char* StateManager::getStateName() {
    return getStateName(currentState);
}

const char* StateManager::getStateName(SystemState state) {
    switch (state) {
        case STATE_BOOT:            return "BOOT";
        case STATE_READY:           return "READY";
        case STATE_HOMING:          return "HOMING";
        case STATE_TESTING:         return "TESTING";
        case STATE_RUNNING_TIDE:    return "RUNNING_TIDE";
        case STATE_FETCHING_DATA:   return "FETCHING_DATA";
        case STATE_EMERGENCY_STOP:  return "EMERGENCY_STOP";
        case STATE_ERROR:           return "ERROR";
        default:                    return "UNKNOWN";
    }
}

bool StateManager::canHome() {
    return (currentState == STATE_READY);
}

bool StateManager::canTest() {
    return (currentState == STATE_READY);
}

bool StateManager::canChangeConfig() {
    return (currentState == STATE_READY ||
            currentState == STATE_ERROR ||
            currentState == STATE_EMERGENCY_STOP);
}

void StateManager::enterEmergencyStop() {
    setState(STATE_EMERGENCY_STOP);
    Logger::warning(CAT_SYSTEM, "EMERGENCY STOP ACTIVATED");
}

void StateManager::clearEmergencyStop() {
    if (currentState == STATE_EMERGENCY_STOP) {
        setState(STATE_READY);
        Logger::info(CAT_SYSTEM, "Emergency stop cleared - System ready");
    }
}

void StateManager::enterError(const char* errorMsg) {
    strncpy(errorMessage, errorMsg, sizeof(errorMessage) - 1);
    errorMessage[sizeof(errorMessage) - 1] = '\0';
    setState(STATE_ERROR);
    Logger::error(CAT_SYSTEM, errorMsg);
}

void StateManager::clearError() {
    if (currentState == STATE_ERROR) {
        errorMessage[0] = '\0';
        setState(STATE_READY);
        Logger::info(CAT_SYSTEM, "Error cleared - System ready");
    }
}

unsigned long StateManager::getStateTimestamp() {
    return stateTimestamp;
}

const char* StateManager::getErrorMessage() {
    return errorMessage;
}
