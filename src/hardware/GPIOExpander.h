/**
 * GPIO Expander Manager
 *
 * Manages all MCP23017 GPIO expander boards with error handling and retry logic.
 * Provides high-level interface for pin operations on all 5 boards.
 */

#ifndef GPIO_EXPANDER_H
#define GPIO_EXPANDER_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include "../config.h"
#include "../utils/Logger.h"

class GPIOExpander {
public:
    /**
     * Initialize all MCP23017 boards
     * @return true if all boards initialized successfully
     */
    static bool begin();

    /**
     * Write a digital value to a specific pin on a specific board
     * @param address I2C address of the MCP23017
     * @param pin Pin number (0-15)
     * @param value HIGH or LOW
     * @return true if successful, false on error
     */
    static bool digitalWrite(uint8_t address, uint8_t pin, uint8_t value);

    /**
     * Read a digital value from a specific pin on a specific board
     * @param address I2C address of the MCP23017
     * @param pin Pin number (0-15)
     * @param value Reference to store the result (HIGH or LOW)
     * @return true if successful, false on error
     */
    static bool digitalRead(uint8_t address, uint8_t pin, uint8_t& value);

    /**
     * Write multiple pins at once (entire port A or B)
     * @param address I2C address of the MCP23017
     * @param port 0 for Port A (pins 0-7), 1 for Port B (pins 8-15)
     * @param value 8-bit value to write
     * @return true if successful, false on error
     */
    static bool writePort(uint8_t address, uint8_t port, uint8_t value);

    /**
     * Read multiple pins at once (entire port A or B)
     * @param address I2C address of the MCP23017
     * @param port 0 for Port A (pins 0-7), 1 for Port B (pins 8-15)
     * @param value Reference to store the 8-bit result
     * @return true if successful, false on error
     */
    static bool readPort(uint8_t address, uint8_t port, uint8_t& value);

    /**
     * Set pin mode for a specific pin
     * @param address I2C address of the MCP23017
     * @param pin Pin number (0-15)
     * @param mode INPUT, OUTPUT, or INPUT_PULLUP
     * @return true if successful, false on error
     */
    static bool pinMode(uint8_t address, uint8_t pin, uint8_t mode);

    /**
     * Check if all boards are initialized and responding
     * @return true if all boards OK, false otherwise
     */
    static bool healthCheck();

    /**
     * Get pointer to specific MCP instance (for advanced operations)
     * @param address I2C address
     * @return Pointer to Adafruit_MCP23X17 object, or nullptr if not found
     */
    static Adafruit_MCP23X17* getMCP(uint8_t address);

private:
    static Adafruit_MCP23X17 motorBoard0;    // 0x20 - Motors 0-7
    static Adafruit_MCP23X17 motorBoard1;    // 0x21 - Motors 8-15
    static Adafruit_MCP23X17 motorBoard2;    // 0x22 - Motors 16-23
    static Adafruit_MCP23X17 switchBoard0;   // 0x23 - Switches 0-15
    static Adafruit_MCP23X17 switchBoard1;   // 0x24 - Switches 16-23

    static bool initialized;

    /**
     * Get MCP instance pointer by address
     */
    static Adafruit_MCP23X17* getBoardByAddress(uint8_t address);

    /**
     * Retry an operation with exponential backoff
     */
    template<typename Func>
    static bool retryOperation(const char* opName, Func operation);
};

#endif // GPIO_EXPANDER_H
