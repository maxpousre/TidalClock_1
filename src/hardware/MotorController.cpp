/**
 * Motor Controller Implementation
 */

#include "MotorController.h"

bool MotorController::initialized = false;
bool MotorController::emergencyStop = false;

bool MotorController::begin() {
    Logger::info(CAT_MOTOR, "Initializing Motor Controller...");

    // Motor boards should already be initialized by GPIOExpander
    // We just ensure all motors are stopped

    emergencyStopAll();
    clearEmergencyStop();  // Clear the flag after stopping

    initialized = true;
    Logger::logf(LOG_INFO, CAT_MOTOR, "Motor Controller initialized: %d motors ready", NUM_MOTORS);

    return true;
}

bool MotorController::isValidIndex(uint8_t motorIndex) {
    if (motorIndex >= NUM_MOTORS) {
        Logger::logf(LOG_ERROR, CAT_MOTOR, "Invalid motor index: %d (must be 0-%d)",
                     motorIndex, NUM_MOTORS - 1);
        return false;
    }
    return true;
}

bool MotorController::setMotorPins(uint8_t motorIndex, uint8_t in1, uint8_t in2) {
    if (!initialized || !isValidIndex(motorIndex)) {
        return false;
    }

    // Get the pin mapping for this motor
    MotorPinMap pinMap = MOTOR_PIN_MAP[motorIndex];

    // Set IN1 pin
    if (!GPIOExpander::digitalWrite(pinMap.mcpAddress, pinMap.in1Pin, in1)) {
        Logger::logf(LOG_ERROR, CAT_MOTOR, "Failed to set IN1 for motor %d", motorIndex);
        return false;
    }

    // Set IN2 pin
    if (!GPIOExpander::digitalWrite(pinMap.mcpAddress, pinMap.in2Pin, in2)) {
        Logger::logf(LOG_ERROR, CAT_MOTOR, "Failed to set IN2 for motor %d", motorIndex);
        return false;
    }

    return true;
}

bool MotorController::setMotorDirection(uint8_t motorIndex, MotorDirection direction) {
    if (emergencyStop) {
        Logger::warning(CAT_MOTOR, "Cannot control motor: Emergency stop active");
        return false;
    }

    if (!initialized || !isValidIndex(motorIndex)) {
        return false;
    }

    uint8_t in1, in2;

    switch (direction) {
        case MOTOR_STOP:
            in1 = LOW;
            in2 = LOW;
            break;

        case MOTOR_FORWARD:
            in1 = HIGH;
            in2 = LOW;
            break;

        case MOTOR_REVERSE:
            in1 = LOW;
            in2 = HIGH;
            break;

        default:
            Logger::logf(LOG_ERROR, CAT_MOTOR, "Invalid direction: %d", direction);
            return false;
    }

    Logger::logf(LOG_DEBUG, CAT_MOTOR, "Motor %d: %s (IN1=%s, IN2=%s)",
                 motorIndex, getDirectionString(direction),
                 in1 ? "HIGH" : "LOW", in2 ? "HIGH" : "LOW");

    return setMotorPins(motorIndex, in1, in2);
}

bool MotorController::runMotorForward(uint8_t motorIndex, uint16_t durationMs) {
    Logger::logf(LOG_INFO, CAT_MOTOR, "Running motor %d FORWARD for %d ms", motorIndex, durationMs);

    if (!setMotorDirection(motorIndex, MOTOR_FORWARD)) {
        return false;
    }

    delay(durationMs);

    if (!stopMotor(motorIndex)) {
        return false;
    }

    Logger::logf(LOG_INFO, CAT_MOTOR, "Motor %d forward run complete", motorIndex);
    return true;
}

bool MotorController::runMotorReverse(uint8_t motorIndex, uint16_t durationMs) {
    Logger::logf(LOG_INFO, CAT_MOTOR, "Running motor %d REVERSE for %d ms", motorIndex, durationMs);

    if (!setMotorDirection(motorIndex, MOTOR_REVERSE)) {
        return false;
    }

    delay(durationMs);

    if (!stopMotor(motorIndex)) {
        return false;
    }

    Logger::logf(LOG_INFO, CAT_MOTOR, "Motor %d reverse run complete", motorIndex);
    return true;
}

bool MotorController::stopMotor(uint8_t motorIndex) {
    Logger::logf(LOG_DEBUG, CAT_MOTOR, "Stopping motor %d", motorIndex);
    return setMotorDirection(motorIndex, MOTOR_STOP);
}

void MotorController::emergencyStopAll() {
    Logger::warning(CAT_MOTOR, "*** EMERGENCY STOP ACTIVATED ***");
    emergencyStop = true;

    // Force all motors to stop immediately
    for (uint8_t i = 0; i < NUM_MOTORS; i++) {
        setMotorPins(i, LOW, LOW);
    }

    Logger::info(CAT_MOTOR, "All motors stopped");
}

bool MotorController::isEmergencyStopped() {
    return emergencyStop;
}

void MotorController::clearEmergencyStop() {
    emergencyStop = false;
    Logger::info(CAT_MOTOR, "Emergency stop cleared - operations resumed");
}

bool MotorController::releaseFromSwitch(uint8_t motorIndex) {
    // Check if switch is already triggered
    if (SwitchReader::isSwitchTriggered(motorIndex)) {
        Logger::logf(LOG_INFO, CAT_HOMING, "Motor %d switch already triggered, releasing...", motorIndex);

        // Run forward briefly to release
        if (!setMotorDirection(motorIndex, MOTOR_FORWARD)) {
            return false;
        }

        delay(SWITCH_RELEASE_INITIAL_MS);

        if (!stopMotor(motorIndex)) {
            return false;
        }

        // Verify switch is now released
        delay(50);  // Brief stabilization delay
        if (SwitchReader::isSwitchTriggered(motorIndex)) {
            Logger::logf(LOG_WARNING, CAT_HOMING, "Motor %d switch still triggered after release attempt", motorIndex);
            return false;
        }

        Logger::logf(LOG_INFO, CAT_HOMING, "Motor %d switch released successfully", motorIndex);
    }

    return true;
}

HomingResult MotorController::homeSingleMotor(uint8_t motorIndex) {
    if (emergencyStop) {
        Logger::warning(CAT_HOMING, "Cannot home: Emergency stop active");
        return HOMING_CANCELLED;
    }

    if (!initialized || !isValidIndex(motorIndex)) {
        return HOMING_MOTOR_ERROR;
    }

    Logger::separator();
    Logger::logf(LOG_INFO, CAT_HOMING, "Starting homing sequence for motor %d", motorIndex);

    // Step 1: Release switch if already triggered
    if (!releaseFromSwitch(motorIndex)) {
        Logger::logf(LOG_ERROR, CAT_HOMING, "Motor %d: Failed to release from switch", motorIndex);
        return HOMING_SWITCH_ERROR;
    }

    // Step 2: Run motor in reverse until switch triggers
    Logger::logf(LOG_INFO, CAT_HOMING, "Motor %d: Running reverse to find limit switch...", motorIndex);

    if (!setMotorDirection(motorIndex, MOTOR_REVERSE)) {
        Logger::logf(LOG_ERROR, CAT_HOMING, "Motor %d: Failed to start reverse", motorIndex);
        return HOMING_MOTOR_ERROR;
    }

    // Step 3: Poll switch with timeout
    unsigned long startTime = millis();
    bool switchTriggered = false;

    while ((millis() - startTime) < HOMING_TIMEOUT_MS) {
        // Check for emergency stop
        if (emergencyStop) {
            stopMotor(motorIndex);
            Logger::logf(LOG_WARNING, CAT_HOMING, "Motor %d: Homing cancelled by emergency stop", motorIndex);
            return HOMING_CANCELLED;
        }

        // Check switch state
        if (SwitchReader::isSwitchTriggered(motorIndex)) {
            switchTriggered = true;
            Logger::logf(LOG_INFO, CAT_HOMING, "Motor %d: Limit switch triggered after %lu ms",
                         motorIndex, millis() - startTime);
            break;
        }

        delay(SWITCH_POLL_INTERVAL_MS);
    }

    // Stop motor immediately
    stopMotor(motorIndex);

    // Step 4: Check if we timed out
    if (!switchTriggered) {
        Logger::logf(LOG_ERROR, CAT_HOMING, "Motor %d: TIMEOUT after %d ms - switch not triggered",
                     motorIndex, HOMING_TIMEOUT_MS);
        return HOMING_TIMEOUT;
    }

    // Step 5: Back away from switch
    Logger::logf(LOG_INFO, CAT_HOMING, "Motor %d: Backing away from switch...", motorIndex);

    if (!setMotorDirection(motorIndex, MOTOR_FORWARD)) {
        Logger::logf(LOG_ERROR, CAT_HOMING, "Motor %d: Failed to back away", motorIndex);
        return HOMING_MOTOR_ERROR;
    }

    delay(SWITCH_RELEASE_TIME_MS);

    stopMotor(motorIndex);

    // Step 6: Verify switch is released
    delay(50);  // Brief stabilization
    if (SwitchReader::isSwitchTriggered(motorIndex)) {
        Logger::logf(LOG_ERROR, CAT_HOMING, "Motor %d: Switch still triggered after backing away", motorIndex);
        return HOMING_SWITCH_ERROR;
    }

    Logger::logf(LOG_INFO, CAT_HOMING, "Motor %d: HOMING COMPLETE", motorIndex);
    Logger::separator();

    return HOMING_SUCCESS;
}

uint8_t MotorController::homeAllMotors() {
    Logger::separator();
    Logger::info(CAT_HOMING, "=== STARTING FULL HOMING SEQUENCE ===");
    Logger::logf(LOG_INFO, CAT_HOMING, "Homing all %d motors sequentially...", NUM_MOTORS);
    Logger::separator();

    uint8_t successCount = 0;
    unsigned long totalStartTime = millis();

    for (uint8_t i = 0; i < NUM_MOTORS; i++) {
        if (emergencyStop) {
            Logger::warning(CAT_HOMING, "Homing sequence aborted by emergency stop");
            break;
        }

        HomingResult result = homeSingleMotor(i);

        if (result == HOMING_SUCCESS) {
            successCount++;
        } else {
            Logger::logf(LOG_ERROR, CAT_HOMING, "Motor %d homing failed: %s",
                         i, getHomingResultString(result));
        }

        // Pause between motors (except after last motor)
        if (i < NUM_MOTORS - 1) {
            delay(PAUSE_BETWEEN_MOTORS_MS);
        }
    }

    unsigned long totalTime = millis() - totalStartTime;

    Logger::separator();
    Logger::info(CAT_HOMING, "=== HOMING SEQUENCE COMPLETE ===");
    Logger::logf(LOG_INFO, CAT_HOMING, "Results: %d/%d motors homed successfully", successCount, NUM_MOTORS);
    Logger::logf(LOG_INFO, CAT_HOMING, "Total time: %lu seconds", totalTime / 1000);
    Logger::separator();

    return successCount;
}

const char* MotorController::getHomingResultString(HomingResult result) {
    switch (result) {
        case HOMING_SUCCESS:       return "SUCCESS";
        case HOMING_TIMEOUT:       return "TIMEOUT - switch not reached";
        case HOMING_SWITCH_ERROR:  return "SWITCH ERROR - cannot read switch";
        case HOMING_MOTOR_ERROR:   return "MOTOR ERROR - cannot control motor";
        case HOMING_CANCELLED:     return "CANCELLED - emergency stop";
        default:                   return "UNKNOWN ERROR";
    }
}

const char* MotorController::getDirectionString(MotorDirection dir) {
    switch (dir) {
        case MOTOR_STOP:    return "STOP";
        case MOTOR_FORWARD: return "FORWARD";
        case MOTOR_REVERSE: return "REVERSE";
        default:            return "UNKNOWN";
    }
}
