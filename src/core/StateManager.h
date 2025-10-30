/**
 * TideClock State Manager
 *
 * Manages the system state machine to prevent invalid operations
 * and provide clear status to users via web interface.
 */

#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <Arduino.h>

/**
 * System State Enumeration (Phase 2 subset)
 * Full state machine will be expanded in Phase 4
 */
enum SystemState {
    STATE_BOOT,             // System startup in progress
    STATE_READY,            // Idle, ready for operations
    STATE_HOMING,           // Homing operation in progress
    STATE_TESTING,          // Manual motor testing in progress
    STATE_EMERGENCY_STOP,   // Emergency stop active
    STATE_ERROR             // System error condition

    // Phase 3+ states (not yet implemented):
    // STATE_FETCHING_DATA,
    // STATE_RUNNING_PROGRAM
};

class StateManager {
public:
    /**
     * Initialize the state manager
     */
    static void begin();

    /**
     * Get current system state
     */
    static SystemState getState();

    /**
     * Set system state
     */
    static void setState(SystemState newState);

    /**
     * Get human-readable state name
     */
    static const char* getStateName();
    static const char* getStateName(SystemState state);

    /**
     * Check if operation is allowed in current state
     */
    static bool canHome();
    static bool canTest();
    static bool canChangeConfig();

    /**
     * State transition helpers
     */
    static void enterEmergencyStop();
    static void clearEmergencyStop();
    static void enterError(const char* errorMessage);
    static void clearError();

    /**
     * Get last state change timestamp
     */
    static unsigned long getStateTimestamp();

    /**
     * Get error message (if in ERROR state)
     */
    static const char* getErrorMessage();

private:
    static SystemState currentState;
    static unsigned long stateTimestamp;
    static char errorMessage[128];
};

#endif // STATE_MANAGER_H
