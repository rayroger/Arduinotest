// WiFi and NTP configuration
// Replace with your network credentials
#define WIFI_SSID     "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// NTP servers (up to three can be specified for redundancy)
#define NTP_SERVER1   "pool.ntp.org"
#define NTP_SERVER2   "time.nist.gov"
#define NTP_SERVER3   "time.google.com"

// Timezone offset in seconds from UTC (e.g. UTC+1 = 3600, UTC-5 = -18000)
#define GMT_OFFSET_SEC    0
// Daylight saving time offset in seconds (3600 for +1 hour, 0 to disable)
#define DAYLIGHT_OFFSET_SEC 0
