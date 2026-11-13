#pragma once
/*
 * BreathGuard - anomaly detection thresholds.
 * All values are starting points for a prototype and must be tuned
 * on the real device / test subject. They are NOT clinically validated.
 */
#include <Arduino.h>

// ---------------- Calibration ----------------
constexpr uint32_t CALIB_TIME_MS         = 60000;  // wear mask, breathe normally
constexpr uint32_t SGP30_WARMUP_MS       = 15000;  // SGP30 outputs 400/0 for ~15 s; ignored
constexpr uint8_t  BR_CALIB_MIN_SAMPLES  = 5;      // need this many radar readings for a baseline

// ---------------- Exhalation detection (SGP30 eCO2) ----------------
constexpr uint16_t ECO2_EXHALE_MIN_PPM   = 420;    // eCO2 must exceed this ...
constexpr uint8_t  ECO2_RISE_COUNT       = 2;      // ... and rise this many samples in a row

// ---------------- TVOC (considered only during exhalation) ----------------
constexpr float    VOC_ALPHA_CALIB       = 0.10f;  // baseline EMA during calibration
constexpr float    VOC_ALPHA_RUN         = 0.02f;  // slow baseline adaptation afterwards
constexpr float    VOC_SHIFT_THRESH_PPB  = 80.0f;  // TVOC above baseline by this = abnormal
constexpr uint32_t VOC_HOLD_MS           = 15000;  // clear VOC flag if no exhalation seen this long

// ---------------- Breathing rate (LD6002), abnormal = DECREASE ----------------
constexpr float    BR_LOW_ABS_BPM        = 12.0f;  // below this = abnormal
constexpr bool     BR_CHECK_DROP         = true;   // also compare with calibrated baseline
constexpr float    BR_DROP_RATIO         = 0.70f;  // below 70 % of baseline = abnormal
constexpr bool     BR_CHECK_HIGH         = false;  // optional: flag fast breathing too
constexpr float    BR_HIGH_ABS_BPM       = 25.0f;

constexpr uint32_t BR_STALE_MS           = 5000;   // radar reading older than this = no reading
constexpr bool     BR_INVALID_IS_ABNORMAL = true;  // no valid reading for a long time = abnormal
constexpr uint32_t BR_LOSS_TIMEOUT_MS    = 15000;  // (possible cessation of breathing or lost target)
constexpr uint8_t  BR_CONFIRM_COUNT      = 3;      // consecutive 1 s evaluations before flagging
