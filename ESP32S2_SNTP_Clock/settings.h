/**
 * settings.h
 *
 * Persistent storage for WiFi credentials and timezone settings using the
 * ESP32 Non-Volatile Storage (NVS) via the Arduino Preferences library.
 *
 * Keys stored under the "clock" namespace:
 *   ssid      – WiFi SSID
 *   pass      – WiFi password
 *   gmtOff    – UTC offset in hours  (int8, -12 … +14)
 *   dstOff    – DST offset in hours  (int8, 0 or 1)
 */

#pragma once
#include <Preferences.h>

struct AppSettings {
    String  ssid;
    String  password;
    int8_t  gmtOffsetHours;   // UTC offset in hours
    int8_t  dstOffsetHours;   // DST offset in hours (0 or 1)
};

static Preferences _prefs;

/**
 * Load settings from NVS.
 * Returns true when a non-empty SSID is found (i.e. settings have been saved
 * at least once).
 */
inline bool loadSettings(AppSettings &s) {
    _prefs.begin("clock", /*readOnly=*/true);
    s.ssid           = _prefs.getString("ssid",   "");
    s.password       = _prefs.getString("pass",   "");
    s.gmtOffsetHours = (int8_t)_prefs.getChar("gmtOff", 0);
    s.dstOffsetHours = (int8_t)_prefs.getChar("dstOff", 0);
    _prefs.end();
    return s.ssid.length() > 0;
}

/**
 * Save settings to NVS.
 */
inline void saveSettings(const AppSettings &s) {
    _prefs.begin("clock", /*readOnly=*/false);
    _prefs.putString("ssid",   s.ssid);
    _prefs.putString("pass",   s.password);
    _prefs.putChar("gmtOff",   (int8_t)s.gmtOffsetHours);
    _prefs.putChar("dstOff",   (int8_t)s.dstOffsetHours);
    _prefs.end();
}

/**
 * Erase all stored settings.
 */
inline void clearSettings() {
    _prefs.begin("clock", /*readOnly=*/false);
    _prefs.clear();
    _prefs.end();
}
