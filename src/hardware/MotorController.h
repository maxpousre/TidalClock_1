/**
 * Motor Controller
 *
 * Controls all 24 DC motors through DRV8833 H-bridge drivers.
 * Manages motor direction, timing, homing sequences, and emergency stop.
 */

#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include <Arduino.h>
#include "../config.h"
#include "../utils/Logger.h"
#include "GPIOExpander.h"
#include "SwitchReader.h"

// Motor direction enumeration
enum MotorDirection {
    MOTOR_STOP,     // Both pins LOW (coast)
    MOTOR_FORWARD,  // IN1=HIGH, IN2=LOW
    MOTOR_REVERSE   // IN1=LOW, IN2=HIGH
};

// Homing result codes
enum HomingResult {
    HOMING_SUCCESS,         // Motor homed successfully
    HOMING_TIMEOUT,         // Timeout waiting for switch
    HOMING_SWITCH_ERROR,    // Could not read switch
    HOMING_MOTOR_ERROR,     // Could not control motor
    HOMING_CANCELLED        // Operation cancelled (e.g., emergency stop)
};

class MotorController {
public:
    /**
     * Initialize motor controller (requires GPIOExpander to be initialized first)
     * @return true if successful
     */
    static bool begin();

    /**
     * Set motor direction
     * @param motorIndex Motor number (0-23)
     * @param direction MOTOR_STOP, MOTOR_FORWARD, or MOTOR_REVERSE
     * @return true if successful
     */
    static bool setMotorDirection(uint8_t motorIndex, MotorDirection direction);

    /**
     * Run motor forward for specified duration
     * @param motorIndex Motor number (0-23)
     * @param durationMs Time to run in milliseconds
     * @return true if successful
     */
    static bool runMotorForward(uint8_t motorIndex, uint16_t durationMs);

    /**
     * Run motor reverse for specified duration
     * @param motorIndex Motor number (0-23)
     * @param durationMs Time to run in milliseconds
     * @return true if successful
     */
    static bool runMotorReverse(uint8_t motorIndex, uint16_t durationMs);

    /**
     * Stop a specific motor
     * @param motorIndex Motor number (0-23)
     * @return true if successful
     */
    static bool stopMotor(uint8_t motorIndex);

    /**
     * Emergency stop - immediately halt all motors
     */
    static void emergencyStopAll();

    /**
     * Check if emergency stop is active
     */
    static bool isEmergencyStopped();

    /**
     * Clear emergency stop flag (must be called to resume operations)
     */
    static void clearEmergencyStop();

    /**
     * Home a single motor using its limit switch
     * @param motorIndex Motor number (0-23)
     * @return HomingResult code indicating success or failure type
     */
    static HomingResult homeSingleMotor(uint8_t motorIndex);

    /**
     * Home all motors sequentially
     * @return Number of motors successfully homed
     */
    static uint8_t homeAllMotors();

    /**
     * Phase 3: Run all motors to tide-based positions
     * @param tideData Pointer to TideDataset with 24 hours of position data
     * @param dryRun If true, log positions without moving motors
     * @return true if sequence completed successfully
     */
    static bool runTideSequence(struct TideDataset* tideData, bool dryRun = false);

    /**
     * Get text description of homing result
     */
    static const char* getHomingResultString(HomingResult result);

    /**
     * Get motor direction as string (for debugging)
     */
    static const char* getDirectionString(MotorDirection dir);

private:
    static bool initialized;
    static bool emergencyStop;

    /**
     * Validate motor index
     */
    static bool isValidIndex(uint8_t motorIndex);

    /**
     * Set both motor control pins directly
     */
    static bool setMotorPins(uint8_t motorIndex, uint8_t in1, uint8_t in2);

    /**
     * Release motor from limit switch (if already triggered)
     * @return true if successful or switch was already released
     */
    static bool releaseFromSwitch(uint8_t motorIndex);
};

#endif // MOTOR_CONTROLLER_H
