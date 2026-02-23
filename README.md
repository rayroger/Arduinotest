# Arduinotest
Arduino-based project for automatic time setting via the internet.

---

## ESP32-S2 Mini SNTP Clock

Located in the [`ESP32S2_SNTP_Clock/`](ESP32S2_SNTP_Clock/) folder.

### Description
An Arduino sketch for the **ESP32-S2 Mini (LOLIN S2 Mini)** that:
1. Stores WiFi credentials, UTC offset, and DST setting persistently in flash (NVS).
2. On first boot (or when stored credentials fail) starts a **WiFi Access Point** and serves a **configuration web page**.
3. After configuration, connects to your network, synchronises the clock via **SNTP**, and prints the current time to Serial every second.
4. Keeps the config web page running at the device's IP so settings can be updated at any time without reflashing.
5. Drives an **automatic watch winder** motor on a configurable schedule, with an LM393 light sensor that stops the motor when the watch drawer is closed (dark).

### Hardware
| Item | Details |
|------|---------|
| Board | ESP32-S2 Mini (LOLIN S2 Mini) |
| Connection | USB (Serial monitor at 115200 baud) |
| Motor driver | L9110 dual-channel DC motor module |
| Watch motor | Any 3–12 V DC motor connected to the L9110 channel A |
| Light sensor | LM393 breakout (digital output) |

#### Wiring

| Signal | ESP32-S2 GPIO | L9110 / LM393 pin |
|--------|---------------|-------------------|
| Motor A input 1 (A1A) | 3 | A-1A |
| Motor A input 2 (A1B) | 2 | A-1B |
| Motor B input 1 (B1A) | 8 | B-1A *(optional)* |
| Motor B input 2 (B1B) | 9 | B-1B *(optional)* |
| Light sensor output | 7 | LM393 DO |

> All pin assignments can be changed in `config.h`.

### Software Requirements
- [Arduino IDE](https://www.arduino.cc/en/software) 2.x **or** [PlatformIO](https://platformio.org/)
- Arduino ESP32 core **≥ 2.0.0** – install via Boards Manager (`esp32` by Espressif)
- Libraries (all included in the ESP32 Arduino core – no extra installs needed):
  - `WiFi`, `WebServer`, `Preferences`, `esp_sntp`

### First-Boot Configuration Flow

1. Power on the device. It creates a WiFi AP:
   - **SSID:** `ESP32-Clock-Config`
   - **Password:** `configure`
2. Connect your phone or laptop to that AP.
3. Open **http://192.168.4.1** in a browser.
4. Fill in the form and click **Save & Restart**.

![Config web page showing SSID, Password, UTC Offset and DST fields](docs/webconfig-screenshot.png)

5. The device restarts, connects to your network, and begins printing the synchronised time.

### Reconfiguration
After the device has connected to your network, the same web page is available at its local IP address (shown in the Serial monitor on boot). Navigate there to change any setting; the device restarts automatically after saving.

### Watch Winder

The watch winder task runs on a **configurable interval** and uses the following duty-cycle pattern (adapted from the reference WatchWinder sketch):

| Cycle index (`n % 5`) | Motor state |
|------------------------|-------------|
| 0 | Off (rest) |
| 1 | **Rotate right** (clockwise) |
| 2 | Off (rest) |
| 3 | **Rotate left** (counter-clockwise) |
| 4 | Off (rest) |

The **LM393 light sensor** overrides every cycle: when its output is HIGH (light path blocked, e.g. the watch drawer is closed), the motor is stopped regardless of the duty-cycle step. This prevents unnecessary running when the watch is not being wound.

Both the enable/disable switch and the cycle interval (in seconds) can be changed at runtime from the **Settings** page of the web portal without reflashing.

### Files

| File | Purpose |
|------|---------|
| `ESP32S2_SNTP_Clock.ino` | Main sketch – WiFi connection, SNTP, task registration, main loop |
| `config.h` | Compile-time defaults (NTP servers, AP credentials, motor & sensor pins) |
| `settings.h` | NVS-backed persistent settings (Preferences) |
| `scheduler.h` | Lightweight task scheduler – daily (HH:MM) and interval (millis) tasks |
| `motor.h` | L9110 DC motor driver (`motorA`, `motorB`) |
| `winder.h` | Watch winder duty-cycle logic + LM393 light-sensor integration |
| `webconfig.h` | HTTP config portal (AP mode + STA mode) |
| `html_pages.h` | HTML page templates and option-builder helpers |
| `led.h` | Non-blocking WiFi-status LED driver |
| `suntime.h` | Sunrise / sunset calculator (NOAA simplified algorithm) |

### AP / Portal Defaults (config.h)
```cpp
#define AP_SSID     "ESP32-Clock-Config"
#define AP_PASSWORD "configure"          // "" for open AP
#define WEB_SERVER_PORT 80
```

### Watch Winder Defaults (config.h)
```cpp
#define A1A  3              // Motor A input 1
#define A1B  2              // Motor A input 2
#define B1A  8              // Motor B input 1
#define B1B  9              // Motor B input 2
#define LIGHT_SENSOR_PIN 7  // LM393 digital output
```

### Expected Serial Output
```
=== ESP32-S2 Mini SNTP Clock ===
Connecting to WiFi SSID: MyNetwork
..........
Connected. IP address: 192.168.1.42
Config portal available at http://192.168.1.42
Initialising SNTP…
Waiting for SNTP sync......
Time synchronised successfully.
2026-02-21 22:36:10
[Scheduler] Running: Watch winder
[Winder] cycle=0 light=0
[Winder] motorA - O
2026-02-21 22:36:30
[Scheduler] Running: Watch winder
[Winder] cycle=1 light=0
[Winder] motorA - R
...
```

---

## SmartThings Ecosystem Integration

The ESP32-S2 already exposes a local HTTP API (`/api/status`) and a web portal, which makes it straightforward to bridge into the [Samsung SmartThings](https://www.smartthings.com/) ecosystem. Three practical options are reviewed below, ordered from easiest to most capable.

### Option 1 — SmartThings HTTP Device Handler (simplest)

SmartThings can poll the device's `/api/status` JSON endpoint directly using a **SmartThings Edge driver** (Lua) or a legacy **Device Type Handler** (Groovy).

| Pro | Con |
|-----|-----|
| No firmware changes needed | SmartThings hub must be on the same LAN as the ESP32 |
| Read-only status (time, winder state) immediately available | Write/control requires adding a POST endpoint to the ESP32 |
| Works today with the existing HTTP API | |

**Minimal firmware change:** expose a `/control` POST endpoint (e.g. `wEn=1` / `wEn=0`) so SmartThings can start and stop the winder remotely.

### Option 2 — MQTT Bridge (recommended)

Add an MQTT client (`PubSubClient` library) to the ESP32 firmware. The device publishes status topics and subscribes to command topics. A SmartThings-compatible MQTT broker bridge (e.g. [SmartThings MQTT Bridge](https://github.com/stjohnjohnson/smartthings-mqtt-bridge)) or [Home Assistant](https://www.home-assistant.io/) acting as a relay can forward state changes to SmartThings.

**Suggested topics:**

| Direction | Topic | Payload |
|-----------|-------|---------|
| Publish   | `watchwinder/status` | `{"winder":"RUN_CW","cycles":42}` |
| Subscribe | `watchwinder/command` | `ON` / `OFF` |

**Required library:** `PubSubClient` (available via Arduino Library Manager — no core changes needed).

### Option 3 — Matter over Wi-Fi (future-proof)

The ESP32-S2 supports the [Matter](https://csa-iot.org/all-solutions/matter/) protocol via Espressif's [esp-matter SDK](https://github.com/espressif/esp-matter). SmartThings added native Matter support in 2023, so a Matter-enabled firmware would integrate with SmartThings **without any intermediary hub or bridge**.

| Pro | Con |
|-----|-----|
| Native SmartThings support — no bridge | Requires migrating the build to the esp-matter SDK (PlatformIO recommended) |
| Cross-ecosystem (Apple Home, Google Home, Amazon Alexa) | Significantly larger firmware footprint |
| Local control with no cloud dependency | Initial commissioning requires a Matter-capable controller |

**Recommended device type:** `OnOff` plug or `Generic Switch` — map the winder enable/disable to the on/off cluster.

### Summary

| Option | Effort | SmartThings support | Local control |
|--------|--------|---------------------|---------------|
| HTTP polling (Edge driver) | Low | ✅ | ✅ |
| MQTT bridge | Medium | ✅ (via bridge) | ✅ |
| Matter over Wi-Fi | High | ✅ (native) | ✅ |

For most users **Option 1** (HTTP Edge driver) is the quickest path to visibility, while **Option 2** (MQTT) offers the best balance of control and ecosystem flexibility without a major firmware rewrite.

