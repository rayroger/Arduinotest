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
