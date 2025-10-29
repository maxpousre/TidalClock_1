/**
 * I2C Manager
 *
 * Handles I2C bus initialization, device scanning, and error handling.
 * Provides centralized I2C management for all MCP23017 GPIO expanders.
 */

#ifndef I2C_MANAGER_H
#define I2C_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include "../config.h"
#include "../utils/Logger.h"

class I2CManager {
public:
    /**
     * Initialize the I2C bus with configured pins and frequency
     * @return true if successful, false otherwise
     */
    static bool begin();

    /**
     * Scan the I2C bus for all connected devices
     * @param printResults If true, prints devices to serial
     * @return Number of devices found
     */
    static uint8_t scanBus(bool printResults = true);

    /**
     * Check if a specific device is present on the bus
     * @param address 7-bit I2C address to check
     * @return true if device responds, false otherwise
     */
    static bool isDevicePresent(uint8_t address);

    /**
     * Verify all required MCP23017 devices are present
     * @return true if all 5 devices detected, false otherwise
     */
    static bool verifyAllDevices();

    /**
     * Print detailed I2C bus status
     */
    static void printStatus();

    /**
     * Get error description for I2C error codes
     * @param error Wire endTransmission() return code
     * @return Human-readable error string
     */
    static const char* getErrorString(uint8_t error);

private:
    static bool initialized;
    static const uint8_t REQUIRED_ADDRESSES[5];
};

#endif // I2C_MANAGER_H
