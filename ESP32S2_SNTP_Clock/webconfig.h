/**
 * webconfig.h
 *
 * HTTP web server providing:
 *
 *   GET  /            (STA mode) Live dashboard: local time, sun times, tasks.
 *   GET  /config      (STA mode) Settings form for WiFi / timezone / location.
 *   GET  /            (AP  mode) Settings form (captive-portal, no nav bar).
 *   POST /save        Persist submitted settings and show a confirmation page.
 *   GET  /api/status  JSON status used by the dashboard's 1-second fetch loop.
 *
 * In AP mode every unrecognised URL is redirected to "/" so that mobile
 * devices trigger the captive-portal prompt automatically.
 *
 * Public API
 * ----------
 *   startConfigPortal(bool apMode);   // call once in setup()
 *   handleConfigPortal();             // call every loop() iteration
 *   bool configSaved();               // true after the user submits the form
 *   stopConfigPortal();               // optional clean shutdown
 */

#pragma once
#include <WebServer.h>
#include <WiFi.h>
#include "config.h"
#include "settings.h"
#include "suntime.h"
#include "scheduler.h"
#include "html_pages.h"

// ── Module state ──────────────────────────────────────────────────────────────

static WebServer _server(WEB_SERVER_PORT);
static bool      _configSaved = false;

// ── Internal helpers ──────────────────────────────────────────────────────────

/**
 * Render the settings form, substituting all template placeholders.
 * @param showNav  When true, injects the dashboard / settings navigation bar.
 */
static String buildConfigPage(bool showNav) {
    AppSettings s;
    loadSettings(s);

    const String nav = showNav
        ? "<nav>"
          "<a href=\"/\">&#x1F4CA; Dashboard</a>"
          "<a href=\"/config\" class=\"active\">&#x2699;&#xFE0F; Settings</a>"
          "</nav>"
        : "";

    char latBuf[12], lonBuf[12];
    snprintf(latBuf, sizeof(latBuf), "%.4f", (double)s.latitude);
    snprintf(lonBuf, sizeof(lonBuf), "%.4f", (double)s.longitude);

    String page = FPSTR(CONFIG_HTML);
    page.replace("%NAV%",         nav);
    page.replace("%SSID%",        s.ssid);
    page.replace("%GMT_OPTIONS%", buildGmtOptions(s.gmtOffsetMinutes));
    page.replace("%DST_OPTIONS%", buildDstOptions(s.dstOffsetHours));
    page.replace("%LAT%",         latBuf);
    page.replace("%LON%",         lonBuf);
    return page;
}

/**
 * Escape a string so it is safe to embed as a JSON string value.
 * Handles the characters that must be escaped per RFC 8259.
 */
static String jsonEscape(const String &s) {
    String out;
    out.reserve(s.length());
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

// ── Request handlers ──────────────────────────────────────────────────────────

/** GET /  (STA mode) – serve the live dashboard page. */
static void handleDashboard() {
    _server.send(200, "text/html", FPSTR(DASHBOARD_HTML));
}

/** GET /config  (STA mode) – serve the settings form with navigation. */
static void handleConfigPage() {
    _server.send(200, "text/html", buildConfigPage(/*showNav=*/true));
}

/** GET /  (AP mode) – serve the settings form without navigation. */
static void handleApConfigPage() {
    _server.send(200, "text/html", buildConfigPage(/*showNav=*/false));
}

/** POST /save – validate, persist and confirm the submitted settings. */
static void handleSave() {
    AppSettings s;
    loadSettings(s);   // start from stored values so omitted fields are kept

    if (_server.hasArg("ssid") && _server.arg("ssid").length() > 0)
        s.ssid = _server.arg("ssid");
    if (_server.hasArg("pass") && _server.arg("pass").length() > 0)
        s.password = _server.arg("pass");
    if (_server.hasArg("gmt"))
        s.gmtOffsetMinutes = (int16_t)_server.arg("gmt").toInt();
    if (_server.hasArg("dst"))
        s.dstOffsetHours = (int8_t)_server.arg("dst").toInt();
    if (_server.hasArg("lat"))
        s.latitude  = _server.arg("lat").toFloat();
    if (_server.hasArg("lon"))
        s.longitude = _server.arg("lon").toFloat();

    saveSettings(s);
    _server.send(200, "text/html", FPSTR(SAVED_HTML));
    _configSaved = true;
}

/**
 * GET /api/status – JSON payload consumed by the dashboard every second.
 *
 * Response shape:
 *   {
 *     "time":    "HH:MM:SS",
 *     "date":    "YYYY-MM-DD Weekday",
 *     "sunrise": "HH:MM",   // "--:--" when unavailable
 *     "sunset":  "HH:MM",
 *     "tasks":   [{"name": "…", "sched": "HH:MM"}, …]
 *   }
 */
static void handleApiStatus() {
    struct tm t;
    if (!getLocalTime(&t)) {
        _server.send(503, "application/json",
                     "{\"error\":\"Time not yet available\"}");
        return;
    }

    // ── Formatted time and date ───────────────────────────────────────────────
    char timeBuf[9];   // "HH:MM:SS\0"
    char dateBuf[32];  // "YYYY-MM-DD Weekday\0"
    strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S",     &t);
    strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d %A",  &t);

    // Use the UTC offset the system clock is actually applying right now.
    // tm_gmtoff reflects the SNTP-configured timezone including any DST the
    // OS has determined to be active, so it is always consistent with the
    // displayed local time and avoids adding dstOffsetHours when DST is not
    // actually in effect (e.g. Israel in February = UTC+2, not UTC+3).
    AppSettings s;
    loadSettings(s);

    int   riseMin   = -1, setMin = -1;
    float gmtOffset = (float)t.tm_gmtoff / 3600.0f;
    SunTime::calcSunriseSunset(t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                               s.latitude, s.longitude, gmtOffset,
                               riseMin, setMin);

    // ── Task list ─────────────────────────────────────────────────────────────
    String tasksJson = "[";
    const auto &tasks = _scheduler.tasks();
    for (size_t i = 0; i < tasks.size(); i++) {
        if (i > 0) tasksJson += ',';
        char sched[6];
        snprintf(sched, sizeof(sched), "%02u:%02u",
                 (unsigned)tasks[i].hour, (unsigned)tasks[i].minute);
        tasksJson += "{\"name\":\"";
        tasksJson += jsonEscape(tasks[i].name);
        tasksJson += "\",\"sched\":\"";
        tasksJson += sched;
        tasksJson += "\"}";
    }
    tasksJson += "]";

    // ── Assemble and send ─────────────────────────────────────────────────────
    String json;
    json.reserve(256);
    json  = "{\"time\":\"";    json += timeBuf;
    json += "\",\"date\":\"";    json += dateBuf;
    json += "\",\"sunrise\":\""; json += SunTime::formatMinutes(riseMin);
    json += "\",\"sunset\":\"";  json += SunTime::formatMinutes(setMin);
    json += "\",\"tasks\":";     json += tasksJson;
    json += "}";

    _server.send(200, "application/json", json);
}

// ── Public API ────────────────────────────────────────────────────────────────

/**
 * Start the configuration web server and register URL routes.
 *
 * @param apMode  true  → device acts as an Access Point; the root URL serves
 *                        the settings form (captive-portal behaviour).
 *                false → device is already on a network; the root URL serves
 *                        the live dashboard.
 */
inline void startConfigPortal(bool apMode = true) {
    _configSaved = false;

    if (apMode) {
        WiFi.mode(WIFI_AP);
        if (strlen(AP_PASSWORD) >= 8)
            WiFi.softAP(AP_SSID, AP_PASSWORD);
        else
            WiFi.softAP(AP_SSID);

        Serial.printf("Config portal: connect to WiFi \"%s\" (pass: \"%s\")"
                      " then open http://192.168.4.1\n",
                      AP_SSID, AP_PASSWORD);

        _server.on("/",     HTTP_GET,  handleApConfigPage);
        _server.on("/save", HTTP_POST, handleSave);
        _server.onNotFound([]() {
            // Redirect every unknown URL to "/" for captive-portal detection.
            _server.sendHeader("Location", "/", /*first=*/true);
            _server.send(302, "text/plain", "");
        });
    } else {
        Serial.printf("Web portal available at http://%s\n",
                      WiFi.localIP().toString().c_str());

        _server.on("/",           HTTP_GET,  handleDashboard);
        _server.on("/config",     HTTP_GET,  handleConfigPage);
        _server.on("/save",       HTTP_POST, handleSave);
        _server.on("/api/status", HTTP_GET,  handleApiStatus);
        _server.onNotFound([]() {
            _server.sendHeader("Location", "/", /*first=*/true);
            _server.send(302, "text/plain", "");
        });
    }

    _server.begin();
}

/** Process pending HTTP requests.  Must be called from loop(). */
inline void handleConfigPortal() {
    _server.handleClient();
}

/** Returns true once the user has submitted and saved the settings form. */
inline bool configSaved() {
    return _configSaved;
}

/** Shut down the web server (e.g. just before restarting). */
inline void stopConfigPortal() {
    _server.stop();
}
