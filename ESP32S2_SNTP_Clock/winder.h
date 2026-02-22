/**
 * winder.h
 *
 * Watch winder control integrated into the ESP32-S2 SNTP Clock.
 *
 * Based on https://github.com/rayroger/Arduino-Projects/blob/master/WatchWinder/WatchWinder.ino
 *
 * Motor cycling pattern (fires on every scheduled interval):
 *   Period  = winderCycles * 2 + 3  (default 5 when winderCycles == 1)
 *   Phase 0              →  motorA Pause
 *   Phase 1 … N          →  motorA Right  (clockwise, CW)
 *   Phase N+1            →  motorA Pause
 *   Phase N+2 … 2N+1     →  motorA Left   (counter-clockwise, CCW)
 *   Phase 2N+2           →  motorA Pause  (wraps to 0)
 *
 * The LM393 light sensor overrides the duty cycle: when the sensor output
 * reads HIGH (light path blocked / darkness), the motor is stopped for that
 * cycle.  This prevents the winder from running when, e.g., the watch drawer
 * is closed.
 *
 * Based on https://create.arduino.cc/projecthub/kiranpaul/light-magic-using-lm393-and-arduino-uno-14eadc
 *
 * Usage
 * -----
 *   // Once in setup():
 *   winderSetup();
 *
 *   // Register as an interval task (e.g. every 20 seconds):
 *   _scheduler.addIntervalTask("Watch winder", 20000UL, winderCycle);
 */

#pragma once
#include <Arduino.h>
#include "motor.h"

// ── Module state ──────────────────────────────────────────────────────────────
// All state is modified only from the scheduler callback (single-threaded
// Arduino loop context); no synchronisation is required.

static int         _winderCycleCount = 0;    ///< Total cycles executed since power-on
static char        _winderLastDir    = 'O';  ///< Last direction driven: 'R', 'L', or 'O'
static const char *_winderStatus     = "PAUSE"; ///< Human-readable motor state
static uint8_t     _winderCycles     = 1;    ///< Winding rotations per direction per session

// ── Public API ────────────────────────────────────────────────────────────────

/**
 * Set the number of consecutive CW (and CCW) rotations per winding session.
 * The full cycle period becomes @p c * 2 + 3 scheduler ticks.
 * Valid range 1–10; values outside this range are clamped.
 * Call once from setup() after loading settings.
 */
inline void winderSetCycles(uint8_t c) {
    _winderCycles = (c < 1) ? 1 : (c > 10 ? 10 : c);
}

/**
 * Initialise motor output pins and the light-sensor input pin.
 * Call once from setup() before the scheduler starts.
 */
inline void winderSetup() {
    motorSetup();
    pinMode(LIGHT_SENSOR_PIN, INPUT);
}

/**
 * Execute one winding cycle.  Intended to be used as the callback for an
 * interval task registered with the scheduler.
 *
 * The cycle period is derived from _winderCycles (N):
 *   Period = N * 2 + 3 ticks
 *   Phase 0          → PAUSE
 *   Phase 1 … N      → CW  (motorA 'R')
 *   Phase N+1        → PAUSE
 *   Phase N+2 … 2N+1 → CCW (motorA 'L')
 *   Phase 2N+2       → PAUSE  (wraps to 0 on next cycle)
 *
 * With the default N=1 this reproduces the original 5-tick R/off/L/off/off
 * pattern.
 *
 * H-bridge safety: the motor is always stopped for 500 ms before the
 * direction is reversed (L↔R) to protect the H-bridge from shoot-through.
 *
 * Reads the LM393 digital output:
 *   HIGH – light blocked (dark) → motor off for this cycle.
 *   LOW  – light present        → apply the CW / pause / CCW / pause pattern.
 */
inline void winderCycle() {
    int light = digitalRead(LIGHT_SENSOR_PIN);
    Serial.print("[Winder] cycle=");
    Serial.print(_winderCycleCount);
    Serial.print(" light=");
    Serial.println(light);

    if (light == HIGH) {
        // Darkness – stop motor for this cycle
        if (_winderLastDir != 'O') {
            motorA('O');
            _winderLastDir = 'O';
        }
        _winderStatus = "DARK";
        Serial.println("[Winder] motorA - O (dark)");
    } else {
        // Determine direction from position within the cycle period
        int n      = (int)_winderCycles;
        int period = n * 2 + 3;
        int phase  = _winderCycleCount % period;

        char dir = 'O';
        if (phase >= 1 && phase <= n)
            dir = 'R';
        else if (phase >= n + 2 && phase <= 2 * n + 1)
            dir = 'L';

        // Safe H-bridge reversal: stop for 500 ms before changing direction.
        // This is a brief, infrequent blocking delay (at most once per winding
        // session) and is acceptable in the Arduino single-threaded loop.
        if ((_winderLastDir == 'R' && dir == 'L') ||
            (_winderLastDir == 'L' && dir == 'R')) {
            Serial.println("[Winder] safe stop before reversal");
            motorA('O');
            delay(500);
        }

        motorA(dir);
        _winderLastDir = dir;

        if (dir == 'R') {
            _winderStatus = "RUN_CW";
            Serial.println("[Winder] motorA - R (CW)");
        } else if (dir == 'L') {
            _winderStatus = "RUN_CCW";
            Serial.println("[Winder] motorA - L (CCW)");
        } else {
            _winderStatus = "PAUSE";
            Serial.println("[Winder] motorA - O (pause)");
        }
    }

    _winderCycleCount++;
}

/** @return Total winding cycles executed since power-on. */
inline int winderCycleCount() { return _winderCycleCount; }

/** @return Current motor state string: "RUN_CW", "RUN_CCW", "PAUSE", or "DARK". */
inline const char *winderStatus() { return _winderStatus; }
