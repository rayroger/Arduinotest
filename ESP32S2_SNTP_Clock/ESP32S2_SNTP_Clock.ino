/**
 * ESP32-S2 Mini SNTP Clock
 *
 * Connects to a WiFi network and synchronises the system time using SNTP.
 * WiFi credentials and timezone settings are stored in NVS (Preferences) and
 * can be configured via a built-in web page.
 *
 * First-boot / no-credentials flow:
 *   1. The device starts a WiFi Access Point named "ESP32-Clock-Config".
 *   2. Connect to that AP (password: "configure") and open http://192.168.4.1
 *   3. Fill in the SSID, WiFi password, UTC offset and DST setting, then click
 *      "Save & Restart".
 *   4. The device restarts, connects to your network and starts syncing time.
 *
 * Reconfiguration:
 *   After connecting to your network the config page is still available at
 *   the device's IP address on port 80.
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
#include "settings.h"
#include "webconfig.h"

// ── Forward declarations ──────────────────────────────────────────────────────
static bool tryConnectWiFi(const AppSettings &s);
static void runConfigPortal();
static void initSNTP(const AppSettings &s);
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

    AppSettings settings;
    bool hasCredentials = loadSettings(settings);

    if (!hasCredentials || !tryConnectWiFi(settings)) {
        // No stored credentials, or connection failed → open config portal
        runConfigPortal();
        // runConfigPortal() only returns if the user saved new settings;
        // the device then restarts.
    }

    // Connected – start the config web server in STA mode so the user can
    // reconfigure without reflashing.
    startConfigPortal(/*apMode=*/false);

    initSNTP(settings);
    waitForTimeSync();
    Serial.println("Time synchronised successfully.");
}

// ── loop ──────────────────────────────────────────────────────────────────────
void loop() {
    handleConfigPortal();

    // Restart as soon as the user saves new settings via the web page
    if (configSaved()) {
        Serial.println("New settings saved – restarting…");
        delay(1000);
        ESP.restart();
    }

    printLocalTime();
    delay(1000);
}

// ── helpers ───────────────────────────────────────────────────────────────────

/**
 * Attempt to connect to WiFi using the provided settings.
 * Returns true on success, false on timeout (30 seconds).
 */
static bool tryConnectWiFi(const AppSettings &s) {
    Serial.printf("Connecting to WiFi SSID: %s\n", s.ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(s.ssid.c_str(), s.password.c_str());

    const unsigned long timeout = 30000UL; // 30 seconds
    const unsigned long start   = millis();

    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > timeout) {
            Serial.println("\nWiFi connection timed out.");
            WiFi.disconnect(true);
            return false;
        }
        delay(500);
        Serial.print('.');
    }

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

    while (!configSaved()) {
        handleConfigPortal();
        delay(10);
    }

    Serial.println("Settings saved – restarting…");
    stopConfigPortal();
    delay(500);
    ESP.restart();
}

/**
 * Configure and start the SNTP client using the timezone settings stored in
 * the provided AppSettings.
 */
static void initSNTP(const AppSettings &s) {
    Serial.println("Initialising SNTP…");

    sntp_set_time_sync_notification_cb(sntpSyncCallback);

    long gmtOffsetSec = (long)s.gmtOffsetHours * 3600L;
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

