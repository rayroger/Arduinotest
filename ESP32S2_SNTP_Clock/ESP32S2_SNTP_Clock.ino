/**
 * ESP32-S2 Mini SNTP Clock
 *
 * Connects to a WiFi network and synchronises the system time
 * using SNTP (Simple Network Time Protocol).  The current time is
 * printed to the Serial monitor once per second.
 *
 * Board : ESP32-S2 Mini (LOLIN S2 Mini)
 * Target: Arduino ESP32 core >= 2.x
 *
 * Wiring: none – only USB / Serial is required.
 */

#include <Arduino.h>
#include <WiFi.h>
#include "esp_sntp.h"
#include "time.h"
#include "config.h"

// ── Forward declarations ──────────────────────────────────────────────────────
static void connectWiFi();
static void initSNTP();
static void waitForTimeSync();
static void printLocalTime();

// ── SNTP sync callback ────────────────────────────────────────────────────────
static volatile bool timeSynced = false;

void sntpSyncCallback(struct timeval *tv) {
    timeSynced = true;
}

// ── setup ─────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }
    Serial.println("\n=== ESP32-S2 Mini SNTP Clock ===");

    connectWiFi();
    initSNTP();
    waitForTimeSync();

    Serial.println("Time synchronised successfully.");
}

// ── loop ──────────────────────────────────────────────────────────────────────
void loop() {
    printLocalTime();
    delay(1000);
}

// ── helpers ───────────────────────────────────────────────────────────────────

/**
 * Connect to the WiFi network defined in config.h.
 * Blocks until a connection is established or the device resets after
 * a 30-second timeout.
 */
static void connectWiFi() {
    Serial.printf("Connecting to WiFi SSID: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    const unsigned long timeout = 30000UL; // 30 seconds
    const unsigned long start   = millis();

    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > timeout) {
            Serial.println("WiFi connection timed out. Restarting…");
            ESP.restart();
        }
        delay(500);
        Serial.print('.');
    }

    Serial.printf("\nConnected. IP address: %s\n",
                  WiFi.localIP().toString().c_str());
}

/**
 * Configure and start the SNTP client.
 * Uses up to three NTP servers defined in config.h.
 */
static void initSNTP() {
    Serial.println("Initialising SNTP…");

    // Register callback so we know when the first sync has completed
    sntp_set_time_sync_notification_cb(sntpSyncCallback);

    // configTime sets the timezone and starts SNTP
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC,
               NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
}

/**
 * Block until the SNTP client has obtained a valid time or a
 * 10-second timeout expires.
 */
static void waitForTimeSync() {
    Serial.print("Waiting for SNTP sync");
    const unsigned long timeout = 10000UL; // 10 seconds
    const unsigned long start   = millis();

    while (!timeSynced) {
        if (millis() - start > timeout) {
            Serial.println("\nSNTP sync timed out.");
            return;
        }
        delay(500);
        Serial.print('.');
    }
    Serial.println();
}

/**
 * Read the current local time and print it to Serial in a human-readable
 * format.  Prints a warning if the time has not yet been set.
 */
static void printLocalTime() {
    struct tm timeInfo;
    if (!getLocalTime(&timeInfo)) {
        Serial.println("Failed to obtain time – retrying…");
        return;
    }

    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeInfo);
    Serial.println(buf);
}
