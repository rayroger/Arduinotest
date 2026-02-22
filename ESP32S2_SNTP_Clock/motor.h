/**
 * motor.h
 *
 * L9110 dual-channel DC motor driver.
 *
 * Based on http://robojax.com/learn/arduino/?vid=robojax_L9110_DC_motor
 *
 * Each motor channel uses two GPIO output pins (A1A/A1B for motor A,
 * B1A/B1B for motor B).  Direction is selected by which pin is driven HIGH:
 *
 *   'R' → right / clockwise
 *   'L' → left  / counter-clockwise
 *   any other char → both LOW, motor coasts to a stop
 *
 * Pin assignments are defined in config.h.
 */

#pragma once
#include <Arduino.h>
#include "config.h"

/** Initialise all four motor output pins and leave both motors stopped. */
inline void motorSetup() {
    pinMode(A1A, OUTPUT);
    pinMode(A1B, OUTPUT);
    pinMode(B1A, OUTPUT);
    pinMode(B1B, OUTPUT);
    digitalWrite(A1A, LOW);
    digitalWrite(A1B, LOW);
    digitalWrite(B1A, LOW);
    digitalWrite(B1B, LOW);
}

/**
 * Drive motor A.
 * @param d  'R' = right, 'L' = left, anything else = stop.
 */
inline void motorA(char d) {
    if (d == 'R') {
        digitalWrite(A1A, LOW);
        digitalWrite(A1B, HIGH);
    } else if (d == 'L') {
        digitalWrite(A1A, HIGH);
        digitalWrite(A1B, LOW);
    } else {
        digitalWrite(A1A, LOW);
        digitalWrite(A1B, LOW);
    }
}

/**
 * Drive motor B.
 * @param d  'R' = right, 'L' = left, anything else = stop.
 */
inline void motorB(char d) {
    if (d == 'R') {
        digitalWrite(B1A, LOW);
        digitalWrite(B1B, HIGH);
    } else if (d == 'L') {
        digitalWrite(B1A, HIGH);
        digitalWrite(B1B, LOW);
    } else {
        digitalWrite(B1A, LOW);
        digitalWrite(B1B, LOW);
    }
}
