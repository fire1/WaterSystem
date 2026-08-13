#ifndef Moon_h
#define Moon_h

/**
 * Lightweight topocentric moon position for embedded / host unit tests.
 * Based on truncated Meeus "Astronomical Algorithms" lunar terms.
 *
 * Tide note (M2 semidiurnal constituent, ~12h 25m between high waters):
 * - High tide at lunar upper transit (overhead, HA≈0) and lower culmination
 *   (underfoot/nadir, |HA|≈12h). Low tide midway (|HA|≈6h).
 * - TidRunMode and Moon3Mode use hour angle for M2 tide windows.
 * - Groundwater follows the same M2 frequency but lags; tune SITE_TIDE_LAG_HOURS.
 * - Spring tides (new/full moon) widen the pumping window; neaps at quarters.
 */

#include <math.h>
#include <stdint.h>

namespace moon {

constexpr float kPi = 3.14159265f;
constexpr float DEG2RAD = kPi / 180.f;
constexpr float RAD2DEG = 180.f / kPi;

struct DateTimeUtc {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
};

struct HorizonResult {
  float altitudeDeg;
  float azimuthDeg;
  float phaseFraction; // 0 = new, 0.5 = full, 1 = new
  float hourAngleHours; // local HA in hours (-12 .. +12); 0 = transit, ±12 = nadir
};

inline float degNorm(float deg) {
  while (deg < 0.f)
    deg += 360.f;
  while (deg >= 360.f)
    deg -= 360.f;
  return deg;
}

inline float radNorm(float rad) {
  const float twoPi = 2.f * kPi;
  while (rad < 0.f)
    rad += twoPi;
  while (rad >= twoPi)
    rad -= twoPi;
  return rad;
}

/** Day of week: 0 = Sunday … 6 = Saturday (Gregorian). */
inline uint8_t dayOfWeek(uint16_t year, uint8_t month, uint8_t day) {
  uint8_t m = month;
  uint16_t y = year;
  if (m < 3) {
    m += 12;
    y--;
  }
  uint16_t k = y % 100;
  uint16_t j = y / 100;
  int h = (day + (13 * (m + 1)) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
  return (uint8_t)((h + 6) % 7); // convert to Sunday = 0
}

/** Last Sunday of a month (EU DST boundary helper). */
inline uint8_t lastSundayOfMonth(uint16_t year, uint8_t month) {
  uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  const bool leap =
      (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
  uint8_t last = daysInMonth[month - 1];
  if (month == 2 && leap)
    last = 29;
  return last - dayOfWeek(year, month, last);
}

/** Bulgaria uses EU DST: last Sunday March – last Sunday October. */
inline bool isBulgariaDst(uint16_t year, uint8_t month, uint8_t day) {
  if (month < 3 || month > 10)
    return false;
  if (month > 3 && month < 10)
    return true;
  if (month == 3)
    return day >= lastSundayOfMonth(year, 3);
  return day < lastSundayOfMonth(year, 10);
}

/** Convert local civil time (Bulgaria) to UTC hours (0–24 fraction ok). */
inline float localBulgariaToUtcHours(uint16_t year, uint8_t month, uint8_t day,
                                     uint8_t hour, uint8_t minute) {
  const int offset = isBulgariaDst(year, month, day) ? 3 : 2;
  float local = (float)hour + (float)minute / 60.f;
  float utc = local - (float)offset;
  if (utc < 0.f)
    utc += 24.f;
  if (utc >= 24.f)
    utc -= 24.f;
  return utc;
}

inline float julianDay(uint16_t year, uint8_t month, uint8_t day,
                       float hourUtc) {
  uint16_t y = year;
  uint8_t m = month;
  if (m <= 2) {
    y--;
    m += 12;
  }
  const int A = y / 100;
  const int B = 2 - A + A / 4;
  const float jd0 = (float)((int)(365.25f * (y + 4716)) +
                            (int)(30.6001f * (m + 1)) + day + B - 1524.5f);
  return jd0 + hourUtc / 24.f;
}

inline void sunEcliptic(float jd, float &lonDeg, float &latDeg) {
  const float T = (jd - 2451545.0f) / 36525.f;
  const float L0 = degNorm(280.46646f + 36000.76983f * T);
  const float M = (357.52911f + 35999.05029f * T) * DEG2RAD;
  const float C = (1.914602f - 0.004817f * T - 0.000014f * T * T) * sinf(M) +
                  (0.019993f - 0.000101f * T) * sinf(2.f * M) +
                  0.000289f * sinf(3.f * M);
  lonDeg = degNorm(L0 + C);
  latDeg = 0.f;
}

inline void moonEcliptic(float jd, float &lonDeg, float &latDeg) {
  const float T = (jd - 2451545.0f) / 36525.f;
  const float Lp = (218.3164477f + 481267.88123421f * T) * DEG2RAD;
  const float D = (297.8501921f + 445267.1114034f * T) * DEG2RAD;
  const float M = (357.5291092f + 35999.0502909f * T) * DEG2RAD;
  const float Mp = (134.9633964f + 477198.8675055f * T) * DEG2RAD;
  const float F = (93.2720950f + 483202.0175233f * T) * DEG2RAD;

  float lon = Lp + (6.288774f * sinf(Mp) + 1.274027f * sinf(2.f * D - Mp) +
                    0.658314f * sinf(2.f * D) + 0.213618f * sinf(2.f * Mp) -
                    0.185116f * sinf(M) - 0.114332f * sinf(2.f * F)) *
                       DEG2RAD;

  float lat = (5.128189f * sinf(F) + 0.280606f * sinf(Mp + F) +
               0.277693f * sinf(Mp - F) + 0.173238f * sinf(2.f * D - F)) *
              DEG2RAD;

  lonDeg = radNorm(lon) * RAD2DEG;
  latDeg = lat * RAD2DEG;
}

inline void eclipticToEquatorial(float jd, float lonDeg, float latDeg,
                                 float &raHours, float &decDeg) {
  const float T = (jd - 2451545.0f) / 36525.f;
  const float eps = (23.439291f - 0.0130042f * T) * DEG2RAD;
  const float lon = lonDeg * DEG2RAD;
  const float lat = latDeg * DEG2RAD;

  const float sinE = sinf(eps);
  const float cosE = cosf(eps);

  const float x = cosf(lon) * cosf(lat);
  const float y = sinf(lon) * cosf(lat);
  const float z = sinf(lat);

  const float xe = x;
  const float ye = y * cosE - z * sinE;
  const float ze = y * sinE + z * cosE;

  raHours = atan2f(ye, xe) * RAD2DEG / 15.f;
  if (raHours < 0.f)
    raHours += 24.f;
  decDeg = asinf(ze) * RAD2DEG;
}

inline float greenwichMeanSiderealHours(float jd) {
  float gmstDeg = 280.46061837f + 360.98564736629f * (jd - 2451545.0f);
  gmstDeg = gmstDeg - floorf(gmstDeg / 360.f) * 360.f;
  if (gmstDeg < 0.f)
    gmstDeg += 360.f;
  return gmstDeg / 15.f;
}

inline float localSiderealHours(float jd, float lonDeg) {
  float lst = greenwichMeanSiderealHours(jd) + lonDeg / 15.f;
  while (lst < 0.f)
    lst += 24.f;
  while (lst >= 24.f)
    lst -= 24.f;
  return lst;
}

inline float moonPhaseFraction(float moonLonDeg, float sunLonDeg) {
  float phase = (1.f - cosf((moonLonDeg - sunLonDeg) * DEG2RAD)) * 0.5f;
  if (phase < 0.f)
    phase = 0.f;
  if (phase > 1.f)
    phase = 1.f;
  return phase;
}

inline HorizonResult computeHorizon(uint16_t year, uint8_t month, uint8_t day,
                                    uint8_t hourLocal, uint8_t minuteLocal,
                                    float latDeg, float lonDeg) {
  const float utc = localBulgariaToUtcHours(year, month, day, hourLocal,
                                            minuteLocal);
  const float jd = julianDay(year, month, day, utc);

  float moonLon, moonLat, sunLon, sunLat;
  moonEcliptic(jd, moonLon, moonLat);
  sunEcliptic(jd, sunLon, sunLat);

  float ra, dec;
  eclipticToEquatorial(jd, moonLon, moonLat, ra, dec);

  const float lst = localSiderealHours(jd, lonDeg);
  float haHours = lst - ra;
  if (haHours < -12.f)
    haHours += 24.f;
  if (haHours > 12.f)
    haHours -= 24.f;
  const float ha = haHours * 15.f * DEG2RAD;

  const float lat = latDeg * DEG2RAD;
  const float decR = dec * DEG2RAD;

  const float sinAlt =
      sinf(lat) * sinf(decR) + cosf(lat) * cosf(decR) * cosf(ha);
  const float alt = asinf(sinAlt) * RAD2DEG;

  const float cosAzNum = sinf(decR) - sinf(lat) * sinAlt;
  const float cosAzDen = cosf(lat) * cosf(asinf(sinAlt));
  float az = 0.f;
  if (fabsf(cosAzDen) > 1e-6f) {
    az = acosf(cosAzNum / cosAzDen) * RAD2DEG;
    if (sinf(ha) > 0.f)
      az = 360.f - az;
  }

  HorizonResult r;
  r.altitudeDeg = alt;
  r.azimuthDeg = az;
  r.phaseFraction = moonPhaseFraction(moonLon, sunLon);
  r.hourAngleHours = haHours;
  return r;
}

inline float moonAltitudeDeg(uint16_t year, uint8_t month, uint8_t day,
                             uint8_t hourLocal, uint8_t minuteLocal,
                             float latDeg, float lonDeg) {
  return computeHorizon(year, month, day, hourLocal, minuteLocal, latDeg,
                        lonDeg)
      .altitudeDeg;
}

inline float moonHourAngleHours(uint16_t year, uint8_t month, uint8_t day,
                                uint8_t hourLocal, uint8_t minuteLocal,
                                float latDeg, float lonDeg) {
  return computeHorizon(year, month, day, hourLocal, minuteLocal, latDeg,
                        lonDeg)
      .hourAngleHours;
}

inline bool isMoonAboveHorizon(uint16_t year, uint8_t month, uint8_t day,
                               uint8_t hourLocal, uint8_t minuteLocal,
                               float latDeg, float lonDeg,
                               float marginDeg = 3.f) {
  return moonAltitudeDeg(year, month, day, hourLocal, minuteLocal, latDeg,
                         lonDeg) > marginDeg;
}

/** Pump schedule selection shared by moon modes and unit tests. */
struct WellSchedule {
  uint8_t runtime;
  uint16_t breaktime;
};

constexpr uint8_t MOON_RUNTIME = 12;
constexpr uint16_t MOON_BREAK_3H = 168;  // 12 + 168 = 180 min (~3h cycle)
constexpr uint16_t MOON_BREAK_4H = 230;  // same as Hours4Mode

/** M2 lunar semidiurnal period (NOAA/Wikipedia: 12h 25.2 min). */
constexpr uint16_t M2_PERIOD_MIN = 745;
constexpr uint16_t M2_HALF_PERIOD_MIN = 373; // high ↔ low (~6h 12.6 min)

/** Wall-clock minutes for a change in |hour angle| (M2 half-period = 6h HA span). */
inline float haDeltaToMinutes(float deltaHours) {
  return fabsf(deltaHours) * ((float)M2_HALF_PERIOD_MIN / 6.f);
}

inline uint16_t clampBreakMinutes(float minutes, uint16_t lo = 10,
                                  uint16_t hi = 500) {
  if (minutes < (float)lo)
    return lo;
  if (minutes > (float)hi)
    return hi;
  return (uint16_t)(minutes + 0.5f);
}

constexpr float TIDE_WINDOW_BASE_HOURS = 1.25f;  // ±1.25h around transit & nadir
constexpr float TIDE_WINDOW_SPRING_EXTRA_H = 0.5f; // wider window at spring tides
// Tuned for ~6 runs/24h (same ballpark as 4-Hour: 12+230=242 min → 6/day).
// High: ~3–4 runs in M2 peaks; low: ~2–3 runs between peaks.
constexpr uint16_t TIDE_BREAK_HIGH = 128; // 12+128 = 2h20m at tidal peak
constexpr uint16_t TIDE_BREAK_LOW = 405;  // 12+405 = 6h57m between peaks

inline float wrapHourAngle(float haHours) {
  while (haHours < -12.f)
    haHours += 24.f;
  while (haHours > 12.f)
    haHours -= 24.f;
  return haHours;
}

/** Shift HA by site lag (groundwater M2 typically trails moon transit). */
inline float applyTideLag(float haHours, float lagHours) {
  return wrapHourAngle(haHours - lagHours);
}

/** 1 at new/full moon (spring tides), 0 at first/third quarter (neap). */
inline float springTideFactor(float phaseFraction) {
  return 1.f - fabsf(sinf(phaseFraction * 2.f * kPi));
}

inline float tideWindowHours(float phaseFraction) {
  return TIDE_WINDOW_BASE_HOURS +
         springTideFactor(phaseFraction) * TIDE_WINDOW_SPRING_EXTRA_H;
}

/** High water near lunar transit (|HA| small) and nadir (|HA| near 12h). */
inline bool isLunarTideHigh(float hourAngleHours, float windowHours) {
  const float absHa = fabsf(hourAngleHours);
  return absHa <= windowHours || absHa >= (12.f - windowHours);
}

inline bool isLunarTideHighAt(float hourAngleHours, float phaseFraction,
                              float lagHours) {
  const float ha = applyTideLag(hourAngleHours, lagHours);
  return isLunarTideHigh(ha, tideWindowHours(phaseFraction));
}

/** Pump schedule for TidRunMode: shorter cycle at M2 peak, longer rest between. */
inline WellSchedule scheduleForTide(bool rtcConnected, bool tideHigh) {
  if (!rtcConnected || !tideHigh)
    return WellSchedule{MOON_RUNTIME, TIDE_BREAK_LOW};
  return WellSchedule{MOON_RUNTIME, TIDE_BREAK_HIGH};
}

/**
 * Moon3Mode: tide-peak runs with ~3h breaks; mid-gap run at |HA|≈6 during
 * the long low-tide interval, then ~4h-equivalent wait until nadir peak.
 */
inline WellSchedule scheduleForTideMoon4(bool rtcConnected, float hourAngleHours,
                                         float phaseFraction, float lagHours) {
  if (!rtcConnected)
    return WellSchedule{MOON_RUNTIME, MOON_BREAK_4H};

  const float ha = applyTideLag(hourAngleHours, lagHours);
  const float w = tideWindowHours(phaseFraction);
  const float absHa = fabsf(ha);

  if (isLunarTideHigh(ha, w))
    return WellSchedule{MOON_RUNTIME, MOON_BREAK_3H};

  constexpr float midHa = 6.f;
  const float nadirHighStart = 12.f - w;

  if (absHa < midHa)
    return WellSchedule{
        MOON_RUNTIME, clampBreakMinutes(haDeltaToMinutes(midHa - absHa))};

  if (absHa < nadirHighStart)
    return WellSchedule{MOON_RUNTIME,
                        clampBreakMinutes(haDeltaToMinutes(nadirHighStart -
                                                           absHa))};

  return WellSchedule{MOON_RUNTIME, MOON_BREAK_3H};
}

} // namespace moon

#endif
