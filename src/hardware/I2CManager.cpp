/**
 * I2C Manager Implementation
 */

#include "I2CManager.h"

bool I2CManager::initialized = false;

// Array of required MCP23017 I2C addresses
const uint8_t I2CManager::REQUIRED_ADDRESSES[5] = {
    MCP_MOTOR_0,    // 0x20
    MCP_MOTOR_1,    // 0x21
    MCP_MOTOR_2,    // 0x22
    MCP_SWITCH_0,   // 0x23
    MCP_SWITCH_1    // 0x24
};

bool I2CManager::begin() {
    Logger::info(CAT_I2C, "Initializing I2C bus...");

    // Initialize Wire library with custom pins
    Wire.begin(I2C_SDA, I2C_SCL);

    // Set I2C clock frequency
    Wire.setClock(I2C_FREQ);

    Logger::logf(LOG_INFO, CAT_I2C, "I2C configured: SDA=%d, SCL=%d, Freq=%dHz",
                 I2C_SDA, I2C_SCL, I2C_FREQ);

    // Small delay to allow bus to stabilize
    delay(100);

    initialized = true;

    Logger::info(CAT_I2C, "I2C bus initialized successfully");
    return true;
}

uint8_t I2CManager::scanBus(bool printResults) {
    if (!initialized) {
        Logger::error(CAT_I2C, "Cannot scan: I2C not initialized");
        return 0;
    }

    if (printResults) {
        Logger::info(CAT_I2C, "Scanning I2C bus...");
    }

    uint8_t devicesFound = 0;
    uint8_t error;

    for (uint8_t address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            devicesFound++;
            if (printResults) {
                Logger::logf(LOG_INFO, CAT_I2C, "Device found at address 0x%02X", address);
            }
        }
        else if (error == 4) {
            if (printResults) {
                Logger::logf(LOG_ERROR, CAT_I2C, "Unknown error at address 0x%02X", address);
            }
        }
    }

    if (printResults) {
        Logger::logf(LOG_INFO, CAT_I2C, "Scan complete: %d device(s) found", devicesFound);
    }

    return devicesFound;
}

bool I2CManager::isDevicePresent(uint8_t address) {
    if (!initialized) {
        Logger::error(CAT_I2C, "Cannot check device: I2C not initialized");
        return false;
    }

    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
        Logger::logf(LOG_DEBUG, CAT_I2C, "Device 0x%02X present", address);
        return true;
    } else {
        Logger::logf(LOG_WARNING, CAT_I2C, "Device 0x%02X not found: %s",
                     address, getErrorString(error));
        return false;
    }
}

bool I2CManager::verifyAllDevices() {
    if (!initialized) {
        Logger::error(CAT_I2C, "Cannot verify: I2C not initialized");
        return false;
    }

    Logger::info(CAT_I2C, "Verifying all required MCP23017 devices...");

    bool allPresent = true;
    uint8_t foundCount = 0;

    for (uint8_t i = 0; i < 5; i++) {
        uint8_t addr = REQUIRED_ADDRESSES[i];
        bool present = isDevicePresent(addr);

        if (present) {
            foundCount++;
            Logger::logf(LOG_INFO, CAT_I2C, "  [OK] MCP23017 at 0x%02X", addr);
        } else {
            allPresent = false;
            Logger::logf(LOG_ERROR, CAT_I2C, "  [FAIL] MCP23017 at 0x%02X NOT FOUND", addr);
        }
    }

    if (allPresent) {
        Logger::logf(LOG_INFO, CAT_I2C, "All %d required devices verified", foundCount);
    } else {
        Logger::logf(LOG_ERROR, CAT_I2C, "Device verification failed: %d/5 found", foundCount);
    }

    return allPresent;
}

void I2CManager::printStatus() {
    Logger::separator();
    Logger::info(CAT_I2C, "I2C BUS STATUS");
    Logger::separator();

    if (!initialized) {
        Logger::error(CAT_I2C, "I2C bus not initialized");
        Logger::separator();
        return;
    }

    Logger::logf(LOG_INFO, CAT_I2C, "Bus Configuration:");
    Logger::logf(LOG_INFO, CAT_I2C, "  SDA Pin: GPIO %d", I2C_SDA);
    Logger::logf(LOG_INFO, CAT_I2C, "  SCL Pin: GPIO %d", I2C_SCL);
    Logger::logf(LOG_INFO, CAT_I2C, "  Frequency: %d Hz", I2C_FREQ);
    Logger::info(CAT_I2C, "");

    Logger::info(CAT_I2C, "Required Devices:");
    Logger::info(CAT_I2C, "  0x20 - Motor Board 0 (Motors 0-7)");
    Logger::info(CAT_I2C, "  0x21 - Motor Board 1 (Motors 8-15)");
    Logger::info(CAT_I2C, "  0x22 - Motor Board 2 (Motors 16-23)");
    Logger::info(CAT_I2C, "  0x23 - Switch Board 0 (Switches 0-15)");
    Logger::info(CAT_I2C, "  0x24 - Switch Board 1 (Switches 16-23)");
    Logger::info(CAT_I2C, "");

    // Perform scan
    scanBus(true);

    Logger::separator();
}

const char* I2CManager::getErrorString(uint8_t error) {
    switch (error) {
        case 0: return "Success";
        case 1: return "Data too long for transmit buffer";
        case 2: return "NACK on transmit of address";
        case 3: return "NACK on transmit of data";
        case 4: return "Other error";
        case 5: return "Timeout";
        default: return "Unknown error";
    }
}
