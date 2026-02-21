/**
 * webconfig.h
 *
 * Provides a captive-portal-style web page that lets the user configure:
 *   - WiFi SSID and password
 *   - UTC (GMT) offset in hours
 *   - DST offset in hours
 *
 * Usage
 * -----
 *   startConfigPortal();          // call once to start AP + web server
 *   handleConfigPortal();         // call every loop() iteration
 *   bool done = configSaved();    // true after the user submits the form
 *
 * After configSaved() returns true the caller should:
 *   1. Read the new settings via loadSettings().
 *   2. Stop the portal with stopConfigPortal().
 *   3. Reconnect to WiFi using the new credentials.
 *
 * The portal is also available while in STA mode so the user can
 * reconfigure the device by navigating to its IP address.
 */

#pragma once
#include <WebServer.h>
#include <WiFi.h>
#include "config.h"
#include "settings.h"

static WebServer  _server(WEB_SERVER_PORT);
static bool       _configSaved = false;

// ── HTML helpers ──────────────────────────────────────────────────────────────

static const char CONFIG_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Clock Setup</title>
<style>
  body{font-family:Arial,sans-serif;background:#f0f2f5;display:flex;
       justify-content:center;align-items:center;min-height:100vh;margin:0}
  .card{background:#fff;border-radius:12px;box-shadow:0 4px 16px rgba(0,0,0,.12);
        padding:2rem;width:min(360px,90vw)}
  h2{margin:0 0 1.4rem;color:#333;text-align:center}
  label{display:block;margin-bottom:.3rem;font-size:.85rem;color:#555}
  input,select{width:100%;box-sizing:border-box;padding:.55rem .7rem;
               border:1px solid #ccc;border-radius:6px;font-size:1rem;
               margin-bottom:1rem}
  input:focus,select:focus{outline:none;border-color:#4a90e2}
  button{width:100%;padding:.7rem;background:#4a90e2;color:#fff;border:none;
         border-radius:6px;font-size:1rem;cursor:pointer}
  button:hover{background:#3a80d2}
  .note{font-size:.75rem;color:#888;margin-top:1rem;text-align:center}
</style>
</head>
<body>
<div class="card">
  <h2>&#x1F552; Clock Setup</h2>
  <form method="POST" action="/save">
    <label for="ssid">WiFi SSID</label>
    <input id="ssid" name="ssid" type="text"
           placeholder="Network name" value="%SSID%" required>

    <label for="pass">WiFi Password</label>
    <input id="pass" name="pass" type="password"
           placeholder="Leave blank to keep current" value="">

    <label for="gmt">UTC Offset (hours)</label>
    <select id="gmt" name="gmt">
      %GMT_OPTIONS%
    </select>

    <label for="dst">Daylight Saving Time</label>
    <select id="dst" name="dst">
      %DST_OPTIONS%
    </select>

    <button type="submit">Save &amp; Restart</button>
  </form>
  <p class="note">The device will restart after saving.</p>
</div>
</body>
</html>
)rawhtml";

static const char SAVED_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Saved</title>
<style>
  body{font-family:Arial,sans-serif;background:#f0f2f5;display:flex;
       justify-content:center;align-items:center;min-height:100vh;margin:0}
  .card{background:#fff;border-radius:12px;box-shadow:0 4px 16px rgba(0,0,0,.12);
        padding:2rem;width:min(360px,90vw);text-align:center}
  h2{color:#4caf50}p{color:#555}
</style>
</head>
<body>
<div class="card">
  <h2>&#x2713; Settings Saved</h2>
  <p>The device is restarting…</p>
</div>
</body>
</html>
)rawhtml";

// ── Build the option lists for the selects ────────────────────────────────────

static String buildGmtOptions(int8_t current) {
    String out;
    for (int8_t h = -12; h <= 14; h++) {
        out += "<option value=\"";
        out += h;
        out += "\"";
        if (h == current) out += " selected";
        out += ">UTC";
        if (h >= 0) out += "+";
        out += h;
        out += "</option>\n";
    }
    return out;
}

static String buildDstOptions(int8_t current) {
    String out;
    const char *labels[] = { "Disabled (0 h)", "Enabled (+1 h)" };
    for (int8_t d = 0; d <= 1; d++) {
        out += "<option value=\"";
        out += d;
        out += "\"";
        if (d == current) out += " selected";
        out += ">";
        out += labels[d];
        out += "</option>\n";
    }
    return out;
}

// ── Request handlers ──────────────────────────────────────────────────────────

static void handleConfigRoot() {
    AppSettings current;
    loadSettings(current);

    String page = FPSTR(CONFIG_HTML);
    page.replace("%SSID%",        current.ssid);
    page.replace("%GMT_OPTIONS%", buildGmtOptions(current.gmtOffsetHours));
    page.replace("%DST_OPTIONS%", buildDstOptions(current.dstOffsetHours));

    _server.send(200, "text/html", page);
}

static void handleConfigSave() {
    AppSettings s;
    loadSettings(s);   // keep existing values as defaults

    if (_server.hasArg("ssid") && _server.arg("ssid").length() > 0) {
        s.ssid = _server.arg("ssid");
    }
    // Only update password when a non-empty value is submitted
    if (_server.hasArg("pass") && _server.arg("pass").length() > 0) {
        s.password = _server.arg("pass");
    }
    if (_server.hasArg("gmt")) {
        s.gmtOffsetHours = (int8_t)_server.arg("gmt").toInt();
    }
    if (_server.hasArg("dst")) {
        s.dstOffsetHours = (int8_t)_server.arg("dst").toInt();
    }

    saveSettings(s);

    _server.send(200, "text/html", FPSTR(SAVED_HTML));
    _configSaved = true;
}

// ── Public API ────────────────────────────────────────────────────────────────

/**
 * Start the configuration web server.
 * In AP mode the device creates its own WiFi network; pass apMode=true.
 * In STA mode the server simply listens on the device's existing IP.
 */
inline void startConfigPortal(bool apMode = true) {
    if (apMode) {
        WiFi.mode(WIFI_AP);
        if (strlen(AP_PASSWORD) >= 8) {
            WiFi.softAP(AP_SSID, AP_PASSWORD);
        } else {
            WiFi.softAP(AP_SSID);
        }
        Serial.printf("Config portal: connect to WiFi \"%s\" (pass: \"%s\")"
                      " then open http://192.168.4.1\n",
                      AP_SSID, AP_PASSWORD);
    } else {
        Serial.printf("Config portal available at http://%s\n",
                      WiFi.localIP().toString().c_str());
    }

    _configSaved = false;
    _server.on("/",      HTTP_GET,  handleConfigRoot);
    _server.on("/save",  HTTP_POST, handleConfigSave);
    _server.onNotFound([]() {
        // Redirect everything else to the config page (captive-portal behaviour)
        _server.sendHeader("Location", "/", /*first=*/true);
        _server.send(302, "text/plain", "");
    });
    _server.begin();
}

/** Must be called from loop() to process incoming HTTP requests. */
inline void handleConfigPortal() {
    _server.handleClient();
}

/** Returns true once the user has submitted the form and settings are saved. */
inline bool configSaved() {
    return _configSaved;
}

/** Stop the web server. */
inline void stopConfigPortal() {
    _server.stop();
}
