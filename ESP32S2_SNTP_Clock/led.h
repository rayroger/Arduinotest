/**
 * led.h
 *
 * Non-blocking WiFi status LED driver for the LOLIN S2 Mini.
 *
 * LED modes
 * ---------
 *   LED_OFF        – no WiFi / idle
 *   LED_BLINK_FAST – connecting to WiFi  (~200 ms toggle interval)
 *   LED_BLINK_SLOW – AP config-portal mode (~500 ms toggle interval)
 *   LED_SOLID      – connected to WiFi (steady ON)
 *
 * Usage
 * -----
 *   ledSetup();                    // once in setup()
 *   ledSetMode(LED_BLINK_FAST);    // change state at transition points
 *   ledUpdate();                   // call as often as possible (loop / tight loops)
 */

#pragma once
#include <Arduino.h>
#include "config.h"

// ── LED mode enum ─────────────────────────────────────────────────────────────

enum LedMode {
    LED_OFF,             ///< LED off (no WiFi or idle)
    LED_BLINK_FAST,      ///< Fast blink  – connecting to WiFi     (~200 ms toggle)
    LED_BLINK_SLOW,      ///< Slow blink  – AP config-portal mode  (~500 ms toggle)
    LED_BLINK_VERY_SLOW, ///< Very slow   – WiFi connected / OK     (~2000 ms toggle, saves power)
    LED_SOLID            ///< Steady on   – connected to WiFi
};

// ── Module state ──────────────────────────────────────────────────────────────
// All variables are file-scoped (static) – safe for this single-translation-unit
// Arduino sketch, consistent with the existing pattern used in settings.h and
// scheduler.h.

static LedMode       _ledMode    = LED_OFF;
static bool          _ledState   = false;
static unsigned long _ledLastMs  = 0;

// ── Public API ────────────────────────────────────────────────────────────────

/** Initialise the LED pin. Call once from setup(). */
inline void ledSetup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
}

/**
 * Switch to a new LED mode.
 * Immediately applies the output for non-blinking modes.
 */
inline void ledSetMode(LedMode mode) {
    if (_ledMode == mode) return;
    _ledMode   = mode;
    _ledLastMs = millis();
    if (mode == LED_SOLID) {
        _ledState = true;
        digitalWrite(LED_PIN, HIGH);
    } else if (mode == LED_OFF) {
        _ledState = false;
        digitalWrite(LED_PIN, LOW);
    } else {
        // Blinking mode: always start from OFF so the first blink is visible.
        _ledState = false;
        digitalWrite(LED_PIN, LOW);
    }
}

/**
 * Drive LED blinking.  Call as frequently as possible – safe to call every
 * loop() iteration.  No-op for LED_SOLID and LED_OFF.
 */
inline void ledUpdate() {
    if (_ledMode != LED_BLINK_FAST &&
        _ledMode != LED_BLINK_SLOW &&
        _ledMode != LED_BLINK_VERY_SLOW) return;

    unsigned int interval = (_ledMode == LED_BLINK_FAST)      ? 200u
                          : (_ledMode == LED_BLINK_VERY_SLOW) ? 2000u
                          :                                      500u;
    unsigned long now = millis();
    if (now - _ledLastMs >= interval) {
        _ledLastMs = now;
        _ledState  = !_ledState;
        digitalWrite(LED_PIN, _ledState ? HIGH : LOW);
    }
}