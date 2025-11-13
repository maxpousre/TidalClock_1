/**
 * TideClock LED Controller
 *
 * Controls WS2812B LED strip via FastLED library
 * Supports static colors and test patterns with time-based control
 */

#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>
#include <FastLED.h>
#include "../config.h"

// Predefined color palette (RGB values)
struct LEDColor {
    const char* name;
    CRGB rgb;
};

// 12 predefined colors for static mode
const LEDColor PREDEFINED_COLORS[] = {
    {"Warm White",  CRGB(255, 220, 180)},  // 0
    {"Cool White",  CRGB(200, 220, 255)},  // 1
    {"Red",         CRGB(255, 0, 0)},      // 2
    {"Orange",      CRGB(255, 128, 0)},    // 3
    {"Yellow",      CRGB(255, 200, 0)},    // 4
    {"Green",       CRGB(0, 255, 0)},      // 5
    {"Cyan",        CRGB(0, 255, 200)},    // 6
    {"Blue",        CRGB(0, 100, 255)},    // 7
    {"Purple",      CRGB(180, 0, 255)},    // 8
    {"Magenta",     CRGB(255, 0, 128)},    // 9
    {"Ocean Blue",  CRGB(0, 120, 180)},    // 10
    {"Deep Teal",   CRGB(0, 80, 100)}      // 11
};

#define NUM_COLORS (sizeof(PREDEFINED_COLORS) / sizeof(LEDColor))

// Test pattern types
enum TestPattern {
    TEST_RGB_CHASE = 0,      // Moving pixel cycling through R/G/B
    TEST_COLOR_CYCLE = 1,    // All LEDs same color, cycling rainbow
    TEST_SEGMENT = 2         // Strip divided into colored segments
};

class LEDController {
public:
    /**
     * Initialize LED controller with current configuration
     * Returns true if initialization successful
     */
    static bool begin();

    /**
     * Reinitialize LED strip with new pin/count settings
     * Called when configuration changes
     */
    static bool reinit(uint8_t pin, uint16_t count);

    /**
     * Update LED display (call from main loop)
     * Handles time-based control and rendering
     */
    static void update();

    /**
     * Set LED system enabled/disabled
     */
    static void setEnabled(bool enabled);

    /**
     * Get LED system enabled state
     */
    static bool isEnabled();

    /**
     * Set brightness (0-255, will be capped at LED_MAX_BRIGHTNESS)
     */
    static void setBrightness(uint8_t brightness);

    /**
     * Set display mode (LED_MODE_STATIC or LED_MODE_TEST)
     */
    static void setMode(uint8_t mode);

    /**
     * Set static color by index (0-11)
     */
    static void setColorIndex(uint8_t colorIndex);

    /**
     * Set active hours
     */
    static void setActiveHours(uint8_t startHour, uint8_t endHour);

    /**
     * Force test pattern mode temporarily (for testing)
     */
    static void runTestPattern();

    /**
     * Get current LED status string
     */
    static const char* getStatusString();

    /**
     * Check if currently within active hours
     */
    static bool isWithinActiveHours();

private:
    // LED strip state
    static CRGB* leds;
    static uint16_t numLEDs;
    static uint8_t dataPin;
    static bool initialized;
    static bool enabled;

    // Display settings
    static uint8_t brightness;
    static uint8_t mode;
    static uint8_t colorIndex;
    static uint8_t startHour;
    static uint8_t endHour;

    // Frame rate control
    static unsigned long lastUpdate;

    // Test pattern state
    static TestPattern currentTestPattern;
    static unsigned long lastPatternChange;
    static uint16_t testAnimationState;

    // Rendering functions
    static void renderStaticColor();
    static void renderTestPattern();
    static void renderRGBChase();
    static void renderColorCycle();
    static void renderSegmentTest();

    // Fade control
    static uint8_t currentBrightness;
    static unsigned long fadeStartTime;
    static bool fading;

    // Helper functions
    static void fadeToTarget(uint8_t targetBrightness);
    static void clearStrip();
};

#endif // LED_CONTROLLER_H
