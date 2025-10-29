/**
 * Logger Implementation
 */

#include "Logger.h"
#include <stdarg.h>

void Logger::begin() {
    Serial.begin(SERIAL_BAUD_RATE);
    // Wait for serial port to connect (useful for debugging)
    delay(500);
}

void Logger::log(LogLevel level, LogCategory category, const char* message) {
    if (!shouldLog(level, category)) {
        return;
    }

    // Format: [LEVEL][CATEGORY] message
    Serial.print(getLevelPrefix(level));
    Serial.print(getCategoryPrefix(category));
    Serial.print(" ");
    Serial.println(message);
}

void Logger::logf(LogLevel level, LogCategory category, const char* format, ...) {
    if (!shouldLog(level, category)) {
        return;
    }

    // Print prefix
    Serial.print(getLevelPrefix(level));
    Serial.print(getCategoryPrefix(category));
    Serial.print(" ");

    // Print formatted message
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Serial.println(buffer);
}

void Logger::error(LogCategory category, const char* message) {
    log(LOG_ERROR, category, message);
}

void Logger::warning(LogCategory category, const char* message) {
    log(LOG_WARNING, category, message);
}

void Logger::info(LogCategory category, const char* message) {
    log(LOG_INFO, category, message);
}

void Logger::debug(LogCategory category, const char* message) {
    log(LOG_DEBUG, category, message);
}

void Logger::verbose(LogCategory category, const char* message) {
    log(LOG_VERBOSE, category, message);
}

void Logger::hexDump(LogCategory category, const uint8_t* data, size_t length) {
    Serial.print("[HEX]");
    Serial.print(getCategoryPrefix(category));
    Serial.print(" ");

    for (size_t i = 0; i < length; i++) {
        if (data[i] < 0x10) {
            Serial.print("0");
        }
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

void Logger::separator() {
    Serial.println("================================================================================");
}

void Logger::printBootHeader() {
    separator();
    Serial.println("  _____ _     _      _____ _            _    ");
    Serial.println(" |_   _(_) __| | ___|  ___| | ___   ___| | __");
    Serial.println("   | | | |/ _` |/ _ \\ |   | |/ _ \\ / __| |/ /");
    Serial.println("   | | | | (_| |  __/ |___| | (_) | (__|   < ");
    Serial.println("   |_| |_|\\__,_|\\___|_____|_|\\___/ \\___|_|\\_\\");
    Serial.println();
    Serial.println("  Kinetic Art Tide Display System v1.0");
    separator();
    Serial.print("  Compiled: ");
    Serial.print(__DATE__);
    Serial.print(" ");
    Serial.println(__TIME__);
    separator();
}

const char* Logger::getLevelPrefix(LogLevel level) {
    switch (level) {
        case LOG_ERROR:   return "[ERROR]";
        case LOG_WARNING: return "[WARN ]";
        case LOG_INFO:    return "[INFO ]";
        case LOG_DEBUG:   return "[DEBUG]";
        case LOG_VERBOSE: return "[TRACE]";
        default:          return "[?????]";
    }
}

const char* Logger::getCategoryPrefix(LogCategory category) {
    switch (category) {
        case CAT_SYSTEM:  return "[SYSTEM]";
        case CAT_I2C:     return "[I2C   ]";
        case CAT_MOTOR:   return "[MOTOR ]";
        case CAT_SWITCH:  return "[SWITCH]";
        case CAT_HOMING:  return "[HOMING]";
        case CAT_TEST:    return "[TEST  ]";
        default:          return "[??????]";
    }
}

bool Logger::shouldLog(LogLevel level, LogCategory category) {
    #if DEBUG_MODE == 0
        // If debug mode is off, only show errors and warnings
        return (level <= LOG_WARNING);
    #endif

    // Category-specific filtering
    switch (category) {
        case CAT_I2C:
            #if DEBUG_I2C == 0
                return (level <= LOG_WARNING);
            #endif
            break;

        case CAT_MOTOR:
            #if DEBUG_MOTOR == 0
                return (level <= LOG_WARNING);
            #endif
            break;

        case CAT_SWITCH:
            #if DEBUG_SWITCH == 0
                return (level <= LOG_WARNING);
            #endif
            break;

        case CAT_HOMING:
            #if DEBUG_HOMING == 0
                return (level <= LOG_WARNING);
            #endif
            break;

        default:
            break;
    }

    // In debug mode, show everything
    return true;
}
