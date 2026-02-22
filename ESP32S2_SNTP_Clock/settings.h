/**
 * settings.h
 *
 * Persistent storage for all user-configurable settings using the ESP32
 * Non-Volatile Storage (NVS) library (Arduino Preferences wrapper).
 *
 * All values live under the "clock" NVS namespace:
 *
 *   Key      Type    Description
 *   -------  ------  -----------------------------------------
 *   ssid     String  WiFi network name
 *   pass     String  WiFi password
 *   gmtMin   int16   UTC offset in minutes (-720 … +840), e.g. IST = 330
 *   dstOff   int8    DST offset in hours (0 or 1)
 *   lat      float   Latitude  in decimal degrees (°N, -90 … +90)
 *   lon      float   Longitude in decimal degrees (°E, -180 … +180)
 */

#pragma once
#include <Preferences.h>

// ── AppSettings ───────────────────────────────────────────────────────────────

struct AppSettings {
    String  ssid;
    String  password;
    int16_t gmtOffsetMinutes = 0;  ///< UTC offset in minutes, e.g. IST = 330 (+5 h 30 m)
    int8_t  dstOffsetHours   = 0;  ///< DST offset in hours (0 or 1)
    float   latitude         = 0.0f; ///< Decimal degrees north (negative = south)
    float   longitude        = 0.0f; ///< Decimal degrees east  (negative = west)
};

// ── NVS helpers ───────────────────────────────────────────────────────────────

static Preferences _prefs;

/**
 * Load settings from NVS.
 * @return true when a non-empty SSID is present (device has been configured).
 */
inline bool loadSettings(AppSettings &s) {
    _prefs.begin("clock", /*readOnly=*/true);
    s.ssid             = _prefs.getString("ssid",   "");
    s.password         = _prefs.getString("pass",   "");
    s.gmtOffsetMinutes = _prefs.getShort ("gmtMin", 0);
    s.dstOffsetHours   = (int8_t)_prefs.getChar ("dstOff", 0);
    s.latitude         = _prefs.getFloat  ("lat",   0.0f);
    s.longitude        = _prefs.getFloat  ("lon",   0.0f);
    _prefs.end();
    return s.ssid.length() > 0;
}

/** Persist settings to NVS. */
inline void saveSettings(const AppSettings &s) {
    _prefs.begin("clock", /*readOnly=*/false);
    _prefs.putString("ssid",   s.ssid);
    _prefs.putString("pass",   s.password);
    _prefs.putShort ("gmtMin", s.gmtOffsetMinutes);
    _prefs.putChar  ("dstOff", (int8_t)s.dstOffsetHours);
    _prefs.putFloat ("lat",    s.latitude);
    _prefs.putFloat ("lon",    s.longitude);
    _prefs.end();
}

/** Erase all stored settings (factory reset). */
inline void clearSettings() {
    _prefs.begin("clock", /*readOnly=*/false);
    _prefs.clear();
    _prefs.end();
}
