/**
 * winder.h
 *
 * Watch winder control integrated into the ESP32-S2 SNTP Clock.
 *
 * Based on https://github.com/rayroger/Arduino-Projects/blob/master/WatchWinder/WatchWinder.ino
 *
 * Motor cycling pattern (fires on every scheduled interval):
 *   cycleCount % 5 == 1  →  motorA Right   (clockwise)
 *   cycleCount % 5 == 3  →  motorA Left    (counter-clockwise)
 *   all other remainders →  motorA Off     (coast / rest)
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

static int _winderCycleCount = 0; ///< Total cycles executed since power-on

// ── Public API ────────────────────────────────────────────────────────────────

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
 * Reads the LM393 digital output:
 *   HIGH – light blocked (dark) → motor off for this cycle.
 *   LOW  – light present        → apply the R / off / L / off / off pattern.
 */
inline void winderCycle() {
    int light = digitalRead(LIGHT_SENSOR_PIN);
    Serial.print("[Winder] cycle=");
    Serial.print(_winderCycleCount);
    Serial.print(" light=");
    Serial.println(light);

    if (light == HIGH) {
        // Darkness – stop motor for this cycle
        Serial.println("[Winder] motorA - O (dark)");
        motorA('O');
    } else {
        switch (_winderCycleCount % 5) {
            case 1:
                Serial.println("[Winder] motorA - R");
                motorA('R');
                break;
            case 3:
                Serial.println("[Winder] motorA - L");
                motorA('L');
                break;
            default:
                Serial.println("[Winder] motorA - O");
                motorA('O');
                break;
        }
    }

    _winderCycleCount++;
}

/** @return Total winding cycles executed since power-on. */
inline int winderCycleCount() { return _winderCycleCount; }
