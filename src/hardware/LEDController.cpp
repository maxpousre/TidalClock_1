/**
 * TideClock LED Controller Implementation
 */

#include "LEDController.h"
#include "../core/ConfigManager.h"
#include "../utils/Logger.h"
#include "../network/TimeManager.h"

// Static member initialization
CRGB* LEDController::leds = nullptr;
uint16_t LEDController::numLEDs = 0;
uint8_t LEDController::dataPin = 0;
bool LEDController::initialized = false;
bool LEDController::enabled = false;

uint8_t LEDController::brightness = LED_DEFAULT_BRIGHTNESS;
uint8_t LEDController::mode = LED_MODE_STATIC;
uint8_t LEDController::colorIndex = 6;  // Cyan default
uint8_t LEDController::startHour = LED_DEFAULT_START_HOUR;
uint8_t LEDController::endHour = LED_DEFAULT_END_HOUR;

unsigned long LEDController::lastUpdate = 0;

TestPattern LEDController::currentTestPattern = TEST_RGB_CHASE;
unsigned long LEDController::lastPatternChange = 0;
uint16_t LEDController::testAnimationState = 0;

uint8_t LEDController::currentBrightness = 0;
unsigned long LEDController::fadeStartTime = 0;
bool LEDController::fading = false;

bool LEDController::begin() {
    Logger::info(CAT_SYSTEM, "Initializing LED Controller...");

    // Load configuration
    const TideClockConfig& config = ConfigManager::getConfig();

    dataPin = config.ledPin;
    numLEDs = config.ledCount;
    enabled = config.ledEnabled;
    brightness = config.ledBrightness;
    mode = config.ledMode;
    colorIndex = config.ledColorIndex;
    startHour = config.ledStartHour;
    endHour = config.ledEndHour;

    // Validate LED count
    if (numLEDs == 0 || numLEDs > 300) {
        Logger::error(CAT_SYSTEM, "Invalid LED count - LED controller disabled");
        return false;
    }

    // Allocate LED array
    leds = new CRGB[numLEDs];
    if (leds == nullptr) {
        Logger::error(CAT_SYSTEM, "Failed to allocate LED memory!");
        return false;
    }

    // Initialize FastLED
    // Note: FastLED.addLeds is template-based, we use WS2812B
    switch (dataPin) {
        case 15: FastLED.addLeds<WS2812B, 15, GRB>(leds, numLEDs); break;
        case 5:  FastLED.addLeds<WS2812B, 5, GRB>(leds, numLEDs); break;
        case 16: FastLED.addLeds<WS2812B, 16, GRB>(leds, numLEDs); break;
        case 17: FastLED.addLeds<WS2812B, 17, GRB>(leds, numLEDs); break;
        case 18: FastLED.addLeds<WS2812B, 18, GRB>(leds, numLEDs); break;
        case 23: FastLED.addLeds<WS2812B, 23, GRB>(leds, numLEDs); break;
        default:
            Logger::logf(LOG_WARNING, CAT_SYSTEM,
                         "Unsupported LED pin %u - using default template", dataPin);
            FastLED.addLeds<WS2812B, LED_DEFAULT_PIN, GRB>(leds, numLEDs);
            break;
    }

    // Set initial brightness (will be faded if needed)
    FastLED.setBrightness(0);  // Start dark
    FastLED.clear();
    FastLED.show();

    initialized = true;

    Logger::logf(LOG_INFO, CAT_SYSTEM,
                 "LED Controller initialized: %u LEDs on GPIO %u",
                 numLEDs, dataPin);

    return true;
}

bool LEDController::reinit(uint8_t pin, uint16_t count) {
    Logger::info(CAT_SYSTEM, "Reinitializing LED Controller...");

    // Clear existing strip
    if (initialized) {
        clearStrip();
        FastLED.clear();
    }

    // Free old LED array
    if (leds != nullptr) {
        delete[] leds;
        leds = nullptr;
    }

    initialized = false;
    dataPin = pin;
    numLEDs = count;

    // Reinitialize
    return begin();
}

void LEDController::update() {
    if (!initialized) {
        return;
    }

    // Frame rate limiting (~33 FPS)
    unsigned long now = millis();
    if (now - lastUpdate < LED_UPDATE_INTERVAL_MS) {
        return;
    }
    lastUpdate = now;

    // Check if within active hours
    bool shouldBeOn = enabled && isWithinActiveHours();

    // Handle fading
    if (fading) {
        unsigned long fadeElapsed = now - fadeStartTime;
        if (fadeElapsed >= LED_FADE_DURATION_MS) {
            // Fade complete
            currentBrightness = shouldBeOn ? brightness : 0;
            fading = false;
        } else {
            // Calculate fade progress
            float progress = (float)fadeElapsed / LED_FADE_DURATION_MS;
            if (shouldBeOn) {
                // Fading in
                currentBrightness = (uint8_t)(brightness * progress);
            } else {
                // Fading out
                currentBrightness = (uint8_t)(brightness * (1.0 - progress));
            }
        }
        FastLED.setBrightness(currentBrightness);
    } else {
        // Check if we need to start a fade
        uint8_t targetBrightness = shouldBeOn ? brightness : 0;
        if (currentBrightness != targetBrightness) {
            fading = true;
            fadeStartTime = now;
        }
    }

    // Render appropriate pattern
    if (!shouldBeOn || currentBrightness == 0) {
        // LEDs should be off
        clearStrip();
    } else {
        // Render based on mode
        if (mode == LED_MODE_STATIC) {
            renderStaticColor();
        } else if (mode == LED_MODE_TEST) {
            renderTestPattern();
        }
    }

    FastLED.show();
}

void LEDController::setEnabled(bool en) {
    enabled = en;
    Logger::logf(LOG_INFO, CAT_SYSTEM, "LED system %s", en ? "enabled" : "disabled");
}

bool LEDController::isEnabled() {
    return enabled;
}

void LEDController::setBrightness(uint8_t bright) {
    // Enforce maximum brightness cap
    if (bright > LED_MAX_BRIGHTNESS) {
        bright = LED_MAX_BRIGHTNESS;
    }
    brightness = bright;
    Logger::logf(LOG_INFO, CAT_SYSTEM, "LED brightness set to %u", bright);
}

void LEDController::setMode(uint8_t m) {
    if (m > LED_MODE_TEST) {
        return;
    }
    mode = m;
    testAnimationState = 0;  // Reset animation state
    Logger::logf(LOG_INFO, CAT_SYSTEM, "LED mode set to %u", m);
}

void LEDController::setColorIndex(uint8_t idx) {
    if (idx >= NUM_COLORS) {
        return;
    }
    colorIndex = idx;
    Logger::logf(LOG_INFO, CAT_SYSTEM, "LED color set to %s", PREDEFINED_COLORS[idx].name);
}

void LEDController::setActiveHours(uint8_t start, uint8_t end) {
    startHour = start;
    endHour = end;
    Logger::logf(LOG_INFO, CAT_SYSTEM,
                 "LED active hours: %02u:00 - %02u:00", start, end);
}

void LEDController::runTestPattern() {
    // Temporarily switch to test mode
    uint8_t oldMode = mode;
    mode = LED_MODE_TEST;
    currentTestPattern = TEST_RGB_CHASE;
    testAnimationState = 0;

    // The update() function will handle rendering
    // This is just a trigger, mode will be restored by user

    Logger::info(CAT_SYSTEM, "Test pattern activated");
}

const char* LEDController::getStatusString() {
    if (!initialized) {
        return "Not Initialized";
    }
    if (!enabled) {
        return "Disabled";
    }
    if (!isWithinActiveHours()) {
        return "Outside Active Hours";
    }
    return "Active";
}

bool LEDController::isWithinActiveHours() {
    // Get current hour from TimeManager
    time_t now = TimeManager::getCurrentTime();
    struct tm* timeinfo = localtime(&now);
    int currentHour = timeinfo->tm_hour;

    // Handle case where end hour is before start hour (crosses midnight)
    if (endHour <= startHour) {
        // Active hours span midnight (e.g., 22:00 - 08:00)
        return (currentHour >= startHour || currentHour < endHour);
    } else {
        // Normal case (e.g., 08:00 - 22:00)
        return (currentHour >= startHour && currentHour < endHour);
    }
}

// ============================================================================
// RENDERING FUNCTIONS
// ============================================================================

void LEDController::renderStaticColor() {
    // Fill entire strip with selected color
    if (colorIndex >= NUM_COLORS) {
        colorIndex = 0;  // Fallback
    }

    CRGB color = PREDEFINED_COLORS[colorIndex].rgb;
    fill_solid(leds, numLEDs, color);
}

void LEDController::renderTestPattern() {
    unsigned long now = millis();

    // Change test pattern every 5 seconds
    if (now - lastPatternChange >= LED_TEST_PATTERN_INTERVAL_MS) {
        lastPatternChange = now;
        currentTestPattern = (TestPattern)((currentTestPattern + 1) % 3);
        testAnimationState = 0;  // Reset animation

        const char* patternName[] = {"RGB Chase", "Color Cycle", "Segment Test"};
        Logger::logf(LOG_INFO, CAT_SYSTEM, "Test pattern: %s", patternName[currentTestPattern]);
    }

    // Render current test pattern
    switch (currentTestPattern) {
        case TEST_RGB_CHASE:
            renderRGBChase();
            break;
        case TEST_COLOR_CYCLE:
            renderColorCycle();
            break;
        case TEST_SEGMENT:
            renderSegmentTest();
            break;
    }
}

void LEDController::renderRGBChase() {
    // Clear strip
    fill_solid(leds, numLEDs, CRGB::Black);

    // Moving pixel position (wraps around)
    uint16_t pos = testAnimationState % numLEDs;

    // Color cycles every numLEDs * 3 frames (R -> G -> B)
    uint8_t colorPhase = (testAnimationState / numLEDs) % 3;
    CRGB color;
    switch (colorPhase) {
        case 0: color = CRGB::Red; break;
        case 1: color = CRGB::Green; break;
        case 2: color = CRGB::Blue; break;
    }

    // Set the moving pixel
    leds[pos] = color;

    // Increment animation state
    testAnimationState++;
}

void LEDController::renderColorCycle() {
    // All LEDs same color, cycling through rainbow
    uint8_t hue = testAnimationState % 256;
    fill_solid(leds, numLEDs, CHSV(hue, 255, 255));

    // Increment hue (completes cycle in ~10 seconds at 30 FPS)
    testAnimationState = (testAnimationState + 1) % 256;
}

void LEDController::renderSegmentTest() {
    // Divide strip into 4 segments with different colors
    uint16_t segmentSize = numLEDs / 4;

    for (uint16_t i = 0; i < numLEDs; i++) {
        uint8_t segment = i / segmentSize;
        if (segment >= 4) segment = 3;  // Last segment gets remainder

        switch (segment) {
            case 0: leds[i] = CRGB::Red; break;
            case 1: leds[i] = CRGB::Green; break;
            case 2: leds[i] = CRGB::Blue; break;
            case 3: leds[i] = CRGB::White; break;
        }
    }

    // Static pattern, no animation state change needed
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void LEDController::fadeToTarget(uint8_t targetBrightness) {
    // This is handled in update() function via fading flag
    fading = true;
    fadeStartTime = millis();
}

void LEDController::clearStrip() {
    fill_solid(leds, numLEDs, CRGB::Black);
}
