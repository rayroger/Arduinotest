/**
 * ESP32-S2 Mini SNTP Clock
 *
 * Connects to a WiFi network and synchronises the system time using SNTP.
 * WiFi credentials, timezone, and location settings are stored in NVS
 * (Preferences) and can be configured via a built-in web portal.
 *
 * First-boot / no-credentials flow:
 *   1. The device starts a WiFi Access Point named "ESP32-Clock-Config".
 *   2. Connect to that AP (password: "configure") and open http://192.168.4.1
 *   3. Fill in the SSID, WiFi password, UTC offset, DST setting, and your
 *      latitude / longitude, then click "Save & Restart".
 *   4. The device restarts, connects to your network and starts syncing time.
 *
 * Once connected, the web portal is available at the device's IP address:
 *   /            – Live dashboard: local time, sunrise/sunset, scheduled tasks.
 *   /config      – Settings form (WiFi, timezone, location).
 *   /api/status  – JSON status endpoint (consumed by the dashboard page).
 *
 * Board : ESP32-S2 Mini (LOLIN S2 Mini)
 * Target: Arduino ESP32 core >= 2.x
 */

#include <Arduino.h>
#include <WiFi.h>
#include "esp_sntp.h"
#include "time.h"
#include "config.h"
#include "settings.h"
#include "suntime.h"
#include "scheduler.h"
#include "led.h"
#include "winder.h"
#include "webconfig.h"

// ── Forward declarations ──────────────────────────────────────────────────────
static bool tryConnectWiFi(const AppSettings &s);
static void runConfigPortal();
static void initSNTP(const AppSettings &s);
static void waitForTimeSync();
static void printLocalTime();
static void registerTasks();

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
    ledSetup();
    Serial.println("\n=== ESP32-S2 Mini SNTP Clock ===");

    AppSettings settings;
    bool hasCredentials = loadSettings(settings);

    winderSetup();

    if (!hasCredentials || !tryConnectWiFi(settings)) {
        // No stored credentials, or connection failed → open config portal
        runConfigPortal();
        // runConfigPortal() only returns if the user saved new settings;
        // the device then restarts.
    }

    // Connected – start the web portal in STA mode (dashboard + settings).
    startConfigPortal(/*apMode=*/false);

    initSNTP(settings);
    waitForTimeSync();
    Serial.println("Time synchronised successfully.");

    // Register daily scheduled tasks.
    registerTasks();
}

// ── loop ──────────────────────────────────────────────────────────────────────
void loop() {
    handleConfigPortal();

    // Restart as soon as the user saves new settings via the web page.
    if (configSaved()) {
        Serial.println("New settings saved – restarting…");
        delay(1000);
        ESP.restart();
    }

    // Keep LED in sync with the live WiFi connection state.
    // Only call ledSetMode() when the connection state actually changes to
    // avoid redundant WiFi.status() overhead on every 1-second tick.
    static bool wifiWasConnected = false; // false forces ledSetMode on the very first loop iteration
    bool wifiIsConnected = (WiFi.status() == WL_CONNECTED);
    if (wifiIsConnected != wifiWasConnected) {
        wifiWasConnected = wifiIsConnected;
        ledSetMode(wifiIsConnected ? LED_BLINK_VERY_SLOW : LED_OFF);
    }
    ledUpdate();

    _scheduler.run();
    printLocalTime();
    delay(1000);
}

// ── Task registration ─────────────────────────────────────────────────────────

/**
 * Register all scheduled tasks here.
 * Each task fires once per day at the specified local time.
 * Add, remove, or modify tasks freely – they are displayed on the dashboard.
 */
static void registerTasks() {
    _scheduler.addTask("Noon time-check", 12, 0, []() {
        Serial.println("[Task] Noon – daily time check.");
        printLocalTime();
    });

    _scheduler.addTask("Midnight rollover", 0, 0, []() {
        Serial.println("[Task] Midnight – new day started.");
    });

    // Watch winder interval task – fires every winderCycleSec seconds.
    // winderCycle() reads the LM393 light sensor and applies the
    // R / off / L / off / off duty-cycle pattern via the L9110 motor driver.
    AppSettings s;
    loadSettings(s);
    if (s.winderEnabled) {
        winderSetCycles(s.winderCycles);
        _scheduler.addIntervalTask("Watch winder",
                                   (uint32_t)s.winderCycleSec * 1000UL,
                                   winderCycle);
    }
}

// ── WiFi ──────────────────────────────────────────────────────────────────────

/**
 * Attempt to connect to WiFi using the provided settings.
 * @return true on success, false on timeout (30 seconds).
 */
static bool tryConnectWiFi(const AppSettings &s) {
    Serial.printf("Connecting to WiFi SSID: %s\n", s.ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(s.ssid.c_str(), s.password.c_str());

    ledSetMode(LED_BLINK_FAST);

    const unsigned long timeout = 30000UL;
    const unsigned long start   = millis();

    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > timeout) {
            Serial.println("\nWiFi connection timed out.");
            WiFi.disconnect(true);
            ledSetMode(LED_OFF);
            return false;
        }
        delay(500);
        ledUpdate();
        Serial.print('.');
    }

    ledSetMode(LED_SOLID);
    Serial.printf("\nConnected. IP address: %s\n",
                  WiFi.localIP().toString().c_str());
    return true;
}

/**
 * Start the AP-mode config portal and block until the user saves settings,
 * then restart the device so the new settings take effect.
 */
static void runConfigPortal() {
    Serial.println("Starting configuration portal…");
    startConfigPortal(/*apMode=*/true);
    ledSetMode(LED_BLINK_SLOW);

    while (!configSaved()) {
        handleConfigPortal();
        ledUpdate();
        delay(10);
    }

    Serial.println("Settings saved – restarting…");
    stopConfigPortal();
    delay(500);
    ESP.restart();
}

// ── SNTP ──────────────────────────────────────────────────────────────────────

/**
 * Configure and start the SNTP client using the timezone settings from
 * the provided AppSettings.
 */
static void initSNTP(const AppSettings &s) {
    Serial.println("Initialising SNTP…");

    sntp_set_time_sync_notification_cb(sntpSyncCallback);

    long gmtOffsetSec = (long)s.gmtOffsetMinutes * 60L;
    int  dstOffsetSec = (int)s.dstOffsetHours  * 3600;

    configTime(gmtOffsetSec, dstOffsetSec,
               NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
}

/**
 * Block until the SNTP client has obtained a valid time or a
 * 10-second timeout expires.
 */
static void waitForTimeSync() {
    Serial.print("Waiting for SNTP sync");
    const unsigned long timeout = 10000UL;
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

// ── Utilities ─────────────────────────────────────────────────────────────────

/**
 * Read the current local time and print it to Serial in ISO-8601 format.
 * Prints a warning if the time has not yet been set.
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

