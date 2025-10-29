/**
 * Switch Reader Implementation
 */

#include "SwitchReader.h"

bool SwitchReader::initialized = false;

bool SwitchReader::begin() {
    Logger::info(CAT_SWITCH, "Initializing Switch Reader...");

    // Switch boards should already be initialized by GPIOExpander
    // We just verify we can read from them

    bool testStates[NUM_MOTORS];
    uint8_t readCount = readAllSwitches(testStates);

    if (readCount == NUM_MOTORS) {
        initialized = true;
        Logger::logf(LOG_INFO, CAT_SWITCH, "Switch Reader initialized: %d switches ready", NUM_MOTORS);
        return true;
    } else {
        Logger::logf(LOG_ERROR, CAT_SWITCH, "Switch Reader initialization failed: only %d/%d switches readable",
                     readCount, NUM_MOTORS);
        return false;
    }
}

bool SwitchReader::isValidIndex(uint8_t switchIndex) {
    if (switchIndex >= NUM_MOTORS) {
        Logger::logf(LOG_ERROR, CAT_SWITCH, "Invalid switch index: %d (must be 0-%d)",
                     switchIndex, NUM_MOTORS - 1);
        return false;
    }
    return true;
}

bool SwitchReader::isSwitchTriggered(uint8_t switchIndex) {
    if (!initialized || !isValidIndex(switchIndex)) {
        return false;
    }

    uint8_t value;
    if (readSwitch(switchIndex, value)) {
        // Switches are normally-open with pull-up resistors
        // When triggered (closed), they read LOW
        // When not triggered (open), they read HIGH
        bool triggered = (value == LOW);

        Logger::logf(LOG_VERBOSE, CAT_SWITCH, "Switch %d: %s (raw=%s)",
                     switchIndex,
                     triggered ? "TRIGGERED" : "OPEN",
                     value == HIGH ? "HIGH" : "LOW");

        return triggered;
    }

    return false;
}

bool SwitchReader::readSwitch(uint8_t switchIndex, uint8_t& value) {
    if (!initialized || !isValidIndex(switchIndex)) {
        return false;
    }

    // Get the MCP address and pin for this switch
    SwitchPinMap pinMap = SWITCH_PIN_MAP[switchIndex];

    // Read from the appropriate GPIO expander
    return GPIOExpander::digitalRead(pinMap.mcpAddress, pinMap.pin, value);
}

uint8_t SwitchReader::readAllSwitches(bool states[NUM_MOTORS]) {
    if (!initialized) {
        Logger::error(CAT_SWITCH, "Switch Reader not initialized");
        return 0;
    }

    uint8_t successCount = 0;

    Logger::debug(CAT_SWITCH, "Reading all switches...");

    for (uint8_t i = 0; i < NUM_MOTORS; i++) {
        uint8_t value;
        if (readSwitch(i, value)) {
            states[i] = (value == LOW);  // LOW = triggered
            successCount++;
        } else {
            states[i] = false;  // Default to not triggered on error
            Logger::logf(LOG_WARNING, CAT_SWITCH, "Failed to read switch %d", i);
        }
    }

    Logger::logf(LOG_DEBUG, CAT_SWITCH, "Read %d/%d switches successfully", successCount, NUM_MOTORS);

    return successCount;
}

void SwitchReader::printAllSwitches() {
    if (!initialized) {
        Logger::error(CAT_SWITCH, "Switch Reader not initialized");
        return;
    }

    Logger::separator();
    Logger::info(CAT_SWITCH, "LIMIT SWITCH STATUS");
    Logger::separator();

    bool states[NUM_MOTORS];
    readAllSwitches(states);

    // Print in groups of 8 for readability
    for (uint8_t group = 0; group < 3; group++) {
        uint8_t startIdx = group * 8;
        uint8_t endIdx = (group == 2) ? 24 : (startIdx + 8);  // Last group has 8 switches

        Logger::logf(LOG_INFO, CAT_SWITCH, "Switches %02d-%02d:", startIdx, endIdx - 1);

        for (uint8_t i = startIdx; i < endIdx; i++) {
            Logger::logf(LOG_INFO, CAT_SWITCH, "  Switch %02d: %s",
                         i, getStateString(states[i]));
        }

        if (group < 2) {
            Logger::info(CAT_SWITCH, "");  // Blank line between groups
        }
    }

    // Count how many are triggered
    uint8_t triggeredCount = 0;
    for (uint8_t i = 0; i < NUM_MOTORS; i++) {
        if (states[i]) triggeredCount++;
    }

    Logger::info(CAT_SWITCH, "");
    Logger::logf(LOG_INFO, CAT_SWITCH, "Summary: %d triggered, %d open",
                 triggeredCount, NUM_MOTORS - triggeredCount);

    Logger::separator();
}

const char* SwitchReader::getStateString(bool triggered) {
    return triggered ? "TRIGGERED (closed)" : "OPEN";
}
