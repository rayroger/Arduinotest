/**
 * suntime.h
 *
 * Sunrise / sunset calculation using the NOAA simplified solar algorithm.
 *
 * Reference: https://en.wikipedia.org/wiki/Sunrise_equation
 *
 * Usage:
 *   int riseMin, setMin;
 *   SunTime::calcSunriseSunset(year, month, day,
 *                              latitude, longitude, gmtOffsetHours,
 *                              riseMin, setMin);
 *   Serial.println(SunTime::formatMinutes(riseMin)); // e.g. "06:47"
 */

#pragma once
#include <math.h>
#include <Arduino.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace SunTime {

static inline double _deg2rad(double d) { return d * M_PI / 180.0; }
static inline double _rad2deg(double r) { return r * 180.0 / M_PI; }

/** Julian Day Number from a Gregorian calendar date. */
static double _julianDay(int year, int month, int day) {
    if (month <= 2) { year--; month += 12; }
    int A = year / 100;
    int B = 2 - A + A / 4;
    return (int)(365.25 * (year + 4716))
         + (int)(30.6001 * (month + 1))
         + day + B - 1524.5;
}

/**
 * Calculate sunrise and sunset as minutes past midnight (local time).
 *
 * @param year, month, day  Calendar date
 * @param latitude          Degrees north (negative = south)
 * @param longitude         Degrees east  (negative = west)
 * @param gmtOffset         UTC offset in hours, including DST
 * @param sunriseMin [out]  Minutes past midnight for sunrise (-1 = no sunrise)
 * @param sunsetMin  [out]  Minutes past midnight for sunset  (-1 = no sunset)
 * @return true on normal day, false for polar day or polar night
 */
inline bool calcSunriseSunset(int year, int month, int day,
                               float latitude, float longitude,
                               float gmtOffset,
                               int &sunriseMin, int &sunsetMin) {
    double JD    = _julianDay(year, month, day);
    double n     = JD - 2451545.0 + 0.0008;
    double Jstar = n - (double)longitude / 360.0;

    // Solar mean anomaly (degrees)
    double M    = fmod(357.5291 + 0.98560028 * Jstar, 360.0);
    double Mrad = _deg2rad(M);

    // Equation of the centre
    double C = 1.9148 * sin(Mrad)
             + 0.0200 * sin(2.0 * Mrad)
             + 0.0003 * sin(3.0 * Mrad);

    // Ecliptic longitude (degrees)
    double lambda = fmod(M + C + 180.0 + 102.9372, 360.0);
    double lrad   = _deg2rad(lambda);

    // Solar transit (Julian date)
    double Jtransit = 2451545.0 + Jstar
                    + 0.0053 * sin(Mrad)
                    - 0.0069 * sin(2.0 * lrad);

    // Solar declination
    double sinDec = sin(lrad) * sin(_deg2rad(23.4397));
    double cosDec = cos(asin(sinDec));

    // Hour angle at sunrise/sunset (cos of the angle)
    double latRad = _deg2rad((double)latitude);
    double cosW0  = (sin(_deg2rad(-0.833)) - sin(latRad) * sinDec)
                  / (cos(latRad) * cosDec);

    if (cosW0 < -1.0) {
        // Polar day – sun never sets
        sunriseMin = 0;
        sunsetMin  = 1439;
        return false;
    }
    if (cosW0 > 1.0) {
        // Polar night – sun never rises
        sunriseMin = -1;
        sunsetMin  = -1;
        return false;
    }

    double W0    = _rad2deg(acos(cosW0));   // degrees
    double Jrise = Jtransit - W0 / 360.0;
    double Jset  = Jtransit + W0 / 360.0;

    // Convert Julian date → minutes past midnight (local)
    // The fractional part of a JD counts from noon UTC:
    //   frac=0.0 → noon, frac=0.5 → midnight
    auto jdToLocalMin = [&](double jd) -> int {
        double frac   = jd - floor(jd);
        double utcMin = fmod((frac + 0.5) * 1440.0, 1440.0);
        int local = (int)(utcMin + (double)gmtOffset * 60.0 + 0.5);
        return ((local % 1440) + 1440) % 1440;
    };

    sunriseMin = jdToLocalMin(Jrise);
    sunsetMin  = jdToLocalMin(Jset);
    return true;
}

/** Format minutes-past-midnight as "HH:MM". */
inline String formatMinutes(int minutes) {
    if (minutes < 0) return "--:--";
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d",
             (minutes / 60) % 24, minutes % 60);
    return String(buf);
}

} // namespace SunTime
