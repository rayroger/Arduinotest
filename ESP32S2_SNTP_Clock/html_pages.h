/**
 * html_pages.h
 *
 * HTML page templates stored in flash (PROGMEM), together with helper
 * functions that build the dynamic <option> lists for the settings form.
 *
 * Keeping all HTML here isolates presentational concerns from the server
 * routing logic in webconfig.h.  Each template uses simple %-delimited
 * placeholders that webconfig.h substitutes at request time:
 *
 *   %NAV%          – Navigation bar HTML (empty string in AP mode)
 *   %SSID%         – Stored WiFi SSID
 *   %GMT_OPTIONS%  – <option> elements for the UTC-offset <select>
 *   %DST_OPTIONS%  – <option> elements for the DST <select>
 *   %LAT%          – Stored latitude  (decimal degrees)
 *   %LON%          – Stored longitude (decimal degrees)
 */

#pragma once
#include <Arduino.h>

// ── Dashboard page (STA mode, GET /) ─────────────────────────────────────────
//
// All content is loaded dynamically via a 1-second fetch() loop that calls
// GET /api/status and updates the DOM.  No server-side placeholders needed.

static const char DASHBOARD_HTML[] PROGMEM = R"html(
<!DOCTYPE html><html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Clock</title>
<style>
  *{box-sizing:border-box}
  body{font-family:Arial,sans-serif;background:#1a1a2e;color:#eee;
       margin:0;padding:1rem;display:flex;flex-direction:column;align-items:center}
  nav{display:flex;gap:.5rem;margin-bottom:1rem;width:min(700px,100%)}
  nav a{text-decoration:none;padding:.5rem 1rem;background:#16213e;
        color:#4a90e2;border-radius:6px;font-size:.9rem}
  nav a:hover,nav a.active{background:#4a90e2;color:#fff}
  .grid{display:grid;grid-template-columns:1fr 1fr;gap:1rem;width:min(700px,100%)}
  .card{background:#16213e;border-radius:12px;padding:1.5rem;
        box-shadow:0 4px 16px rgba(0,0,0,.3)}
  .card.full{grid-column:1/-1}
  h2{margin:0 0 .5rem;font-size:.85rem;color:#4a90e2;
     text-transform:uppercase;letter-spacing:.08em}
  .big{font-size:2.8rem;font-weight:bold;color:#fff;margin:.3rem 0}
  .sub{font-size:1rem;color:#aaa}
  .sun-row{display:flex;justify-content:space-around;margin-top:.5rem}
  .sun-item{text-align:center}
  .sun-icon{font-size:1.8rem}
  .sun-lbl{font-size:.75rem;color:#aaa;margin:.25rem 0}
  .sun-val{font-size:1.3rem;font-weight:bold}
  table{width:100%;border-collapse:collapse;margin-top:.4rem}
  th,td{padding:.45rem .6rem;text-align:left;
        border-bottom:1px solid #2a2a4a;font-size:.9rem}
  th{color:#4a90e2;font-weight:600}
  .empty{color:#666;font-style:italic}
  @media(max-width:480px){.grid{grid-template-columns:1fr}.card.full{grid-column:1}}
</style>
</head>
<body>
<nav>
  <a href="/" class="active">&#x1F4CA; Dashboard</a>
  <a href="/config">&#x2699;&#xFE0F; Settings</a>
</nav>
<div class="grid">
  <div class="card full">
    <h2>&#x1F552; Local Time</h2>
    <div class="big" id="clk">--:--:--</div>
    <div class="sub" id="dat">Loading&hellip;</div>
  </div>
  <div class="card">
    <h2>&#x2600;&#xFE0F; Sun Times</h2>
    <div class="sun-row">
      <div class="sun-item">
        <div class="sun-icon">&#x1F305;</div>
        <div class="sun-lbl">Sunrise</div>
        <div class="sun-val" id="rise">--:--</div>
      </div>
      <div class="sun-item">
        <div class="sun-icon">&#x1F307;</div>
        <div class="sun-lbl">Sunset</div>
        <div class="sun-val" id="set">--:--</div>
      </div>
    </div>
  </div>
  <div class="card">
    <h2>&#x23F0; Scheduled Tasks</h2>
    <table>
      <thead><tr><th>Time</th><th>Task</th></tr></thead>
      <tbody id="tbdy">
        <tr><td colspan="2" class="empty">No tasks scheduled</td></tr>
      </tbody>
    </table>
  </div>
</div>
<script>
function refresh() {
  fetch('/api/status')
    .then(function(r){ return r.json(); })
    .then(function(d) {
      document.getElementById('clk').textContent  = d.time;
      document.getElementById('dat').textContent  = d.date;
      document.getElementById('rise').textContent = d.sunrise;
      document.getElementById('set').textContent  = d.sunset;
      var tb = document.getElementById('tbdy');
      if (d.tasks && d.tasks.length > 0) {
        tb.innerHTML = d.tasks
          .map(function(t) {
            return '<tr><td>' + t.sched + '</td><td>' + t.name + '</td></tr>';
          })
          .join('');
      }
    })
    .catch(function() {});
}
refresh();
setInterval(refresh, 1000);
</script>
</body>
</html>
)html";

// ── Settings form (AP root + STA /config) ─────────────────────────────────────

static const char CONFIG_HTML[] PROGMEM = R"html(
<!DOCTYPE html><html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Clock &ndash; Settings</title>
<style>
  *{box-sizing:border-box}
  body{font-family:Arial,sans-serif;background:#f0f2f5;
       display:flex;flex-direction:column;
       justify-content:center;align-items:center;min-height:100vh;margin:0;padding:1rem}
  nav{display:flex;gap:.5rem;margin-bottom:1rem;width:min(400px,100%)}
  nav a{text-decoration:none;padding:.5rem 1rem;background:#fff;color:#4a90e2;
        border-radius:6px;font-size:.9rem;box-shadow:0 1px 4px rgba(0,0,0,.1)}
  nav a:hover,nav a.active{background:#4a90e2;color:#fff}
  .card{background:#fff;border-radius:12px;box-shadow:0 4px 16px rgba(0,0,0,.12);
        padding:2rem;width:min(400px,90vw)}
  h2{margin:0 0 1.4rem;color:#333;text-align:center}
  fieldset{border:1px solid #e0e0e0;border-radius:8px;padding:1rem;margin-bottom:1rem}
  legend{font-size:.8rem;font-weight:600;color:#4a90e2;padding:0 .4rem}
  label{display:block;margin-bottom:.25rem;font-size:.85rem;color:#555}
  input,select{width:100%;padding:.5rem .7rem;border:1px solid #ccc;
               border-radius:6px;font-size:1rem;margin-bottom:.85rem}
  input:focus,select:focus{outline:none;border-color:#4a90e2}
  .row2{display:grid;grid-template-columns:1fr 1fr;gap:.5rem}
  .row2 input{margin-bottom:0}
  button{width:100%;padding:.7rem;background:#4a90e2;color:#fff;border:none;
         border-radius:6px;font-size:1rem;cursor:pointer;margin-top:.5rem}
  button:hover{background:#3a80d2}
  .note{font-size:.75rem;color:#888;margin-top:.8rem;text-align:center}
</style>
</head>
<body>
%NAV%
<div class="card">
  <h2>&#x2699;&#xFE0F; Settings</h2>
  <form method="POST" action="/save">
    <fieldset>
      <legend>WiFi</legend>
      <label for="ssid">SSID</label>
      <input id="ssid" name="ssid" type="text"
             placeholder="Network name" value="%SSID%" required>
      <label for="pass">Password</label>
      <input id="pass" name="pass" type="password"
             placeholder="Leave blank to keep current">
    </fieldset>

    <fieldset>
      <legend>Time Zone</legend>
      <label for="gmt">UTC Offset (hours)</label>
      <select id="gmt" name="gmt">%GMT_OPTIONS%</select>
      <label for="dst">Daylight Saving Time</label>
      <select id="dst" name="dst">%DST_OPTIONS%</select>
    </fieldset>

    <fieldset>
      <legend>Location (for sunrise / sunset)</legend>
      <div class="row2">
        <div>
          <label for="lat">Latitude (&deg;N)</label>
          <input id="lat" name="lat" type="number" step="0.0001"
                 min="-90" max="90" value="%LAT%" placeholder="e.g. 51.5">
        </div>
        <div>
          <label for="lon">Longitude (&deg;E)</label>
          <input id="lon" name="lon" type="number" step="0.0001"
                 min="-180" max="180" value="%LON%" placeholder="e.g. -0.1">
        </div>
      </div>
    </fieldset>

    <button type="submit">Save &amp; Restart</button>
  </form>
  <p class="note">The device will restart after saving.</p>
</div>
</body>
</html>
)html";

// ── "Settings saved" confirmation page ────────────────────────────────────────

static const char SAVED_HTML[] PROGMEM = R"html(
<!DOCTYPE html><html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Saved</title>
<style>
  body{font-family:Arial,sans-serif;background:#f0f2f5;
       display:flex;justify-content:center;align-items:center;min-height:100vh;margin:0}
  .card{background:#fff;border-radius:12px;box-shadow:0 4px 16px rgba(0,0,0,.12);
        padding:2rem;width:min(360px,90vw);text-align:center}
  h2{color:#4caf50} p{color:#555}
</style>
</head>
<body>
<div class="card">
  <h2>&#x2713; Settings Saved</h2>
  <p>The device is restarting&hellip;</p>
</div>
</body>
</html>
)html";

// ── Dropdown option builders ───────────────────────────────────────────────────

/** Build <option> elements for the UTC-offset <select> box. */
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

/** Build <option> elements for the DST <select> box. */
static String buildDstOptions(int8_t current) {
    static const char *const labels[] = { "Disabled (0 h)", "Enabled (+1 h)" };
    String out;
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
