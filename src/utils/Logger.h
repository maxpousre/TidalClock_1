/**
 * Logger Utility
 *
 * Provides standardized debug logging with different severity levels
 * and categorical prefixes for easier debugging.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include "../config.h"

/**
 * Log severity levels
 */
enum LogLevel {
    LOG_ERROR,      // Critical errors
    LOG_WARNING,    // Warning messages
    LOG_INFO,       // General information
    LOG_DEBUG,      // Detailed debug information
    LOG_VERBOSE     // Very detailed trace information
};

/**
 * Log categories for filtering output
 */
enum LogCategory {
    CAT_SYSTEM,     // General system messages
    CAT_I2C,        // I2C communication
    CAT_MOTOR,      // Motor operations
    CAT_SWITCH,     // Switch readings
    CAT_HOMING,     // Homing sequences
    CAT_TEST,       // Testing/debug commands
    CAT_WEB         // Web server and API
};

class Logger {
public:
    /**
     * Initialize the logger with serial baud rate
     */
    static void begin();

    /**
     * Log a message with level and category
     */
    static void log(LogLevel level, LogCategory category, const char* message);

    /**
     * Log formatted message (printf style)
     */
    static void logf(LogLevel level, LogCategory category, const char* format, ...);

    /**
     * Convenience methods for different log levels
     */
    static void error(LogCategory category, const char* message);
    static void warning(LogCategory category, const char* message);
    static void info(LogCategory category, const char* message);
    static void debug(LogCategory category, const char* message);
    static void verbose(LogCategory category, const char* message);

    /**
     * Print a formatted hex dump of data (useful for I2C debugging)
     */
    static void hexDump(LogCategory category, const uint8_t* data, size_t length);

    /**
     * Print a separator line for visual clarity
     */
    static void separator();

    /**
     * Print system boot header
     */
    static void printBootHeader();

private:
    static const char* getLevelPrefix(LogLevel level);
    static const char* getCategoryPrefix(LogCategory category);
    static bool shouldLog(LogLevel level, LogCategory category);
};

#endif // LOGGER_H
