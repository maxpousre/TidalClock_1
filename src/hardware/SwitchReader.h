/**
 * Switch Reader
 *
 * Manages reading of all 24 limit switches through MCP23017 GPIO expanders.
 * Provides convenient interface for switch state monitoring.
 */

#ifndef SWITCH_READER_H
#define SWITCH_READER_H

#include <Arduino.h>
#include "../config.h"
#include "../utils/Logger.h"
#include "GPIOExpander.h"

class SwitchReader {
public:
    /**
     * Initialize switch reader (requires GPIOExpander to be initialized first)
     * @return true if successful
     */
    static bool begin();

    /**
     * Read the state of a single limit switch
     * @param switchIndex Switch number (0-23)
     * @return true if switch is triggered (LOW/closed), false if not triggered (HIGH/open)
     */
    static bool isSwitchTriggered(uint8_t switchIndex);

    /**
     * Read the raw digital value of a switch
     * @param switchIndex Switch number (0-23)
     * @param value Reference to store result (HIGH or LOW)
     * @return true if read successful, false on error
     */
    static bool readSwitch(uint8_t switchIndex, uint8_t& value);

    /**
     * Read all 24 switches at once
     * @param states Array of 24 bools to store results
     * @return Number of switches successfully read
     */
    static uint8_t readAllSwitches(bool states[NUM_MOTORS]);

    /**
     * Print status of all switches to serial
     */
    static void printAllSwitches();

    /**
     * Get a text representation of switch state
     * @param triggered true if switch is triggered
     * @return Human-readable string
     */
    static const char* getStateString(bool triggered);

private:
    static bool initialized;

    /**
     * Validate switch index
     */
    static bool isValidIndex(uint8_t switchIndex);
};

#endif // SWITCH_READER_H
