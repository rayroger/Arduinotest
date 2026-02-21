# Arduinotest
Arduino-based project for automatic time setting via the internet.

---

## ESP32-S2 Mini SNTP Clock

Located in the [`ESP32S2_SNTP_Clock/`](ESP32S2_SNTP_Clock/) folder.

### Description
An Arduino sketch for the **ESP32-S2 Mini (LOLIN S2 Mini)** that:
1. Connects to a WiFi network.
2. Synchronises the system clock using **SNTP** (Simple Network Time Protocol).
3. Prints the current date and time to the Serial monitor once per second.

### Hardware
| Item | Details |
|------|---------|
| Board | ESP32-S2 Mini (LOLIN S2 Mini) |
| Connection | USB (Serial monitor at 115200 baud) |
| Wiring | None – USB only |

### Software Requirements
- [Arduino IDE](https://www.arduino.cc/en/software) 2.x **or** [PlatformIO](https://platformio.org/)
- Arduino ESP32 core **≥ 2.0.0** – install via Boards Manager (`esp32` by Espressif)

### Configuration
Edit `ESP32S2_SNTP_Clock/config.h` before uploading:

```cpp
#define WIFI_SSID           "your_wifi_ssid"
#define WIFI_PASSWORD       "your_wifi_password"

#define NTP_SERVER1         "pool.ntp.org"
#define NTP_SERVER2         "time.nist.gov"
#define NTP_SERVER3         "time.google.com"

// UTC offset in seconds, e.g. UTC+1 = 3600
#define GMT_OFFSET_SEC      0
// Daylight saving offset in seconds, e.g. 3600; set 0 to disable
#define DAYLIGHT_OFFSET_SEC 0
```

### Uploading
1. Open `ESP32S2_SNTP_Clock/ESP32S2_SNTP_Clock.ino` in the Arduino IDE.
2. Select **LOLIN S2 Mini** (or **ESP32S2 Dev Module**) as the target board.
3. Set the upload speed to **921600** (default for S2 Mini).
4. Click **Upload**.
5. Open the Serial Monitor at **115200 baud** to see the synchronised time.

### Expected Serial Output
```
=== ESP32-S2 Mini SNTP Clock ===
Connecting to WiFi SSID: your_wifi_ssid
..........
Connected. IP address: 192.168.1.42
Initialising SNTP…
Waiting for SNTP sync......
Time synchronised successfully.
2026-02-21 22:36:10
2026-02-21 22:36:11
...
```
