/**
 * GPIO Expander Implementation
 */

#include "GPIOExpander.h"

// Static member initialization
Adafruit_MCP23X17 GPIOExpander::motorBoard0;
Adafruit_MCP23X17 GPIOExpander::motorBoard1;
Adafruit_MCP23X17 GPIOExpander::motorBoard2;
Adafruit_MCP23X17 GPIOExpander::switchBoard0;
Adafruit_MCP23X17 GPIOExpander::switchBoard1;
bool GPIOExpander::initialized = false;

bool GPIOExpander::begin() {
    Logger::info(CAT_I2C, "Initializing MCP23017 GPIO expanders...");

    bool success = true;

    // Initialize Motor Board 0 (0x20)
    Logger::debug(CAT_I2C, "Initializing Motor Board 0 (0x20)...");
    if (!motorBoard0.begin_I2C(MCP_MOTOR_0)) {
        Logger::error(CAT_I2C, "Failed to initialize Motor Board 0 at 0x20");
        success = false;
    } else {
        // Configure all pins as outputs
        for (uint8_t pin = 0; pin < 16; pin++) {
            motorBoard0.pinMode(pin, OUTPUT);
            motorBoard0.digitalWrite(pin, LOW);  // Start with motors off
        }
        Logger::info(CAT_I2C, "Motor Board 0 initialized (Motors 0-7)");
    }

    // Initialize Motor Board 1 (0x21)
    Logger::debug(CAT_I2C, "Initializing Motor Board 1 (0x21)...");
    if (!motorBoard1.begin_I2C(MCP_MOTOR_1)) {
        Logger::error(CAT_I2C, "Failed to initialize Motor Board 1 at 0x21");
        success = false;
    } else {
        for (uint8_t pin = 0; pin < 16; pin++) {
            motorBoard1.pinMode(pin, OUTPUT);
            motorBoard1.digitalWrite(pin, LOW);
        }
        Logger::info(CAT_I2C, "Motor Board 1 initialized (Motors 8-15)");
    }

    // Initialize Motor Board 2 (0x22)
    Logger::debug(CAT_I2C, "Initializing Motor Board 2 (0x22)...");
    if (!motorBoard2.begin_I2C(MCP_MOTOR_2)) {
        Logger::error(CAT_I2C, "Failed to initialize Motor Board 2 at 0x22");
        success = false;
    } else {
        for (uint8_t pin = 0; pin < 16; pin++) {
            motorBoard2.pinMode(pin, OUTPUT);
            motorBoard2.digitalWrite(pin, LOW);
        }
        Logger::info(CAT_I2C, "Motor Board 2 initialized (Motors 16-23)");
    }

    // Initialize Switch Board 0 (0x23)
    Logger::debug(CAT_I2C, "Initializing Switch Board 0 (0x23)...");
    if (!switchBoard0.begin_I2C(MCP_SWITCH_0)) {
        Logger::error(CAT_I2C, "Failed to initialize Switch Board 0 at 0x23");
        success = false;
    } else {
        // Configure all pins as inputs with pull-ups
        for (uint8_t pin = 0; pin < 16; pin++) {
            switchBoard0.pinMode(pin, INPUT_PULLUP);
        }
        Logger::info(CAT_I2C, "Switch Board 0 initialized (Switches 0-15)");
    }

    // Initialize Switch Board 1 (0x24)
    Logger::debug(CAT_I2C, "Initializing Switch Board 1 (0x24)...");
    if (!switchBoard1.begin_I2C(MCP_SWITCH_1)) {
        Logger::error(CAT_I2C, "Failed to initialize Switch Board 1 at 0x24");
        success = false;
    } else {
        // Only configure pins 0-7 (8 switches on this board) (I edited this like from 'pin < 8' to 'pin < 16' to initialize all pins)
        for (uint8_t pin = 0; pin < 16; pin++) {
            switchBoard1.pinMode(pin, INPUT_PULLUP);
        }
        Logger::info(CAT_I2C, "Switch Board 1 initialized (Switches 16-23)");
    }

    if (success) {
        initialized = true;
        Logger::info(CAT_I2C, "All MCP23017 boards initialized successfully");
    } else {
        Logger::error(CAT_I2C, "Some MCP23017 boards failed to initialize");
    }

    return success;
}

Adafruit_MCP23X17* GPIOExpander::getBoardByAddress(uint8_t address) {
    switch (address) {
        case MCP_MOTOR_0:  return &motorBoard0;
        case MCP_MOTOR_1:  return &motorBoard1;
        case MCP_MOTOR_2:  return &motorBoard2;
        case MCP_SWITCH_0: return &switchBoard0;
        case MCP_SWITCH_1: return &switchBoard1;
        default:
            Logger::logf(LOG_ERROR, CAT_I2C, "Invalid MCP address: 0x%02X", address);
            return nullptr;
    }
}

bool GPIOExpander::digitalWrite(uint8_t address, uint8_t pin, uint8_t value) {
    if (!initialized) {
        Logger::error(CAT_I2C, "GPIO expanders not initialized");
        return false;
    }

    Adafruit_MCP23X17* board = getBoardByAddress(address);
    if (board == nullptr) {
        return false;
    }

    // Retry logic for I2C operations
    for (uint8_t attempt = 0; attempt < I2C_RETRY_ATTEMPTS; attempt++) {
        board->digitalWrite(pin, value);

        // The Adafruit library doesn't return error status, so we assume success
        // unless we catch an exception or see bus issues
        Logger::logf(LOG_VERBOSE, CAT_I2C, "Write: 0x%02X pin %d = %s",
                     address, pin, value ? "HIGH" : "LOW");
        return true;
    }

    Logger::logf(LOG_ERROR, CAT_I2C, "Failed to write pin after %d attempts", I2C_RETRY_ATTEMPTS);
    return false;
}

bool GPIOExpander::digitalRead(uint8_t address, uint8_t pin, uint8_t& value) {
    if (!initialized) {
        Logger::error(CAT_I2C, "GPIO expanders not initialized");
        return false;
    }

    Adafruit_MCP23X17* board = getBoardByAddress(address);
    if (board == nullptr) {
        return false;
    }

    for (uint8_t attempt = 0; attempt < I2C_RETRY_ATTEMPTS; attempt++) {
        value = board->digitalRead(pin);
        Logger::logf(LOG_VERBOSE, CAT_I2C, "Read: 0x%02X pin %d = %s",
                     address, pin, value ? "HIGH" : "LOW");
        return true;
    }

    Logger::logf(LOG_ERROR, CAT_I2C, "Failed to read pin after %d attempts", I2C_RETRY_ATTEMPTS);
    return false;
}

bool GPIOExpander::writePort(uint8_t address, uint8_t port, uint8_t value) {
    if (!initialized) {
        Logger::error(CAT_I2C, "GPIO expanders not initialized");
        return false;
    }

    Adafruit_MCP23X17* board = getBoardByAddress(address);
    if (board == nullptr) {
        return false;
    }

    if (port == 0) {
        board->writeGPIOA(value);
    } else if (port == 1) {
        board->writeGPIOB(value);
    } else {
        Logger::logf(LOG_ERROR, CAT_I2C, "Invalid port: %d (must be 0 or 1)", port);
        return false;
    }

    Logger::logf(LOG_VERBOSE, CAT_I2C, "WritePort: 0x%02X port %d = 0x%02X",
                 address, port, value);
    return true;
}

bool GPIOExpander::readPort(uint8_t address, uint8_t port, uint8_t& value) {
    if (!initialized) {
        Logger::error(CAT_I2C, "GPIO expanders not initialized");
        return false;
    }

    Adafruit_MCP23X17* board = getBoardByAddress(address);
    if (board == nullptr) {
        return false;
    }

    if (port == 0) {
        value = board->readGPIOA();
    } else if (port == 1) {
        value = board->readGPIOB();
    } else {
        Logger::logf(LOG_ERROR, CAT_I2C, "Invalid port: %d (must be 0 or 1)", port);
        return false;
    }

    Logger::logf(LOG_VERBOSE, CAT_I2C, "ReadPort: 0x%02X port %d = 0x%02X",
                 address, port, value);
    return true;
}

bool GPIOExpander::pinMode(uint8_t address, uint8_t pin, uint8_t mode) {
    if (!initialized) {
        Logger::error(CAT_I2C, "GPIO expanders not initialized");
        return false;
    }

    Adafruit_MCP23X17* board = getBoardByAddress(address);
    if (board == nullptr) {
        return false;
    }

    board->pinMode(pin, mode);
    Logger::logf(LOG_DEBUG, CAT_I2C, "PinMode: 0x%02X pin %d set to %s",
                 address, pin,
                 mode == OUTPUT ? "OUTPUT" :
                 mode == INPUT ? "INPUT" : "INPUT_PULLUP");
    return true;
}

bool GPIOExpander::healthCheck() {
    if (!initialized) {
        return false;
    }

    Logger::debug(CAT_I2C, "Performing GPIO expander health check...");

    // Try to read from each board
    uint8_t dummyValue;
    bool allHealthy = true;

    if (!readPort(MCP_MOTOR_0, 0, dummyValue)) allHealthy = false;
    if (!readPort(MCP_MOTOR_1, 0, dummyValue)) allHealthy = false;
    if (!readPort(MCP_MOTOR_2, 0, dummyValue)) allHealthy = false;
    if (!readPort(MCP_SWITCH_0, 0, dummyValue)) allHealthy = false;
    if (!readPort(MCP_SWITCH_1, 0, dummyValue)) allHealthy = false;

    if (allHealthy) {
        Logger::info(CAT_I2C, "Health check passed: All boards responding");
    } else {
        Logger::error(CAT_I2C, "Health check failed: Some boards not responding");
    }

    return allHealthy;
}

Adafruit_MCP23X17* GPIOExpander::getMCP(uint8_t address) {
    return getBoardByAddress(address);
}
