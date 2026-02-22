// NTP servers (up to three can be specified for redundancy)
#define NTP_SERVER1   "pool.ntp.org"
#define NTP_SERVER2   "time.nist.gov"
#define NTP_SERVER3   "time.google.com"

// Access-Point (config portal) defaults
// The ESP32 creates this AP when no WiFi credentials are stored or
// when the stored credentials fail to connect.
#define AP_SSID      "ESP32-Clock-Config"
#define AP_PASSWORD  "configure"   // min 8 chars; set to "" for open AP

// TCP port the configuration web server listens on
#define WEB_SERVER_PORT 80

// Built-in blue LED on the LOLIN S2 Mini (active HIGH)
#define LED_PIN 15

// L9110 dual-channel DC motor driver pins
// Matches the WatchWinder reference sketch (robojax.com L9110 tutorial).
// Adjust to suit your wiring.
#define A1A  3   ///< Motor A input 1
#define A1B  2   ///< Motor A input 2
#define B1A  8   ///< Motor B input 1
#define B1B  9   ///< Motor B input 2

// LM393 light-sensor digital output pin
// HIGH = light path blocked (dark/drawer closed) → motor off
// LOW  = light present                          → normal duty cycle
// See https://create.arduino.cc/projecthub/kiranpaul/light-magic-using-lm393-and-arduino-uno-14eadc
#define LIGHT_SENSOR_PIN  7
