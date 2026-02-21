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

### Hardware
| Item | Details |
|------|---------|
| Board | ESP32-S2 Mini (LOLIN S2 Mini) |
| Connection | USB (Serial monitor at 115200 baud) |
| Wiring | None – USB only |

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

### Files

| File | Purpose |
|------|---------|
| `ESP32S2_SNTP_Clock.ino` | Main sketch – WiFi connection, SNTP, main loop |
| `config.h` | Compile-time defaults (NTP servers, AP name/password) |
| `settings.h` | NVS-backed persistent settings (Preferences) |
| `webconfig.h` | HTTP config portal (AP mode + STA mode) |

### AP / Portal Defaults (config.h)
```cpp
#define AP_SSID     "ESP32-Clock-Config"
#define AP_PASSWORD "configure"          // "" for open AP
#define WEB_SERVER_PORT 80
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
2026-02-21 22:36:11
...
```

