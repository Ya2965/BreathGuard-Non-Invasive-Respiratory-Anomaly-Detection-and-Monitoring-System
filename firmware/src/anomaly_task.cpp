/*
 * Anomaly detection task - calibration + sensor data fusion
 *
 *   TVOC            : only considered during detected exhalation (eCO2 rising)
 *   Breathing rate  : abnormal when it DECREASES (absolute and/or vs. baseline)
 *
 *   neither abnormal -> NORMAL
 *   one abnormal     -> LOW_ANOMALY
 *   both abnormal    -> HIGH_ANOMALY
 */
#include "config.h"
#include "anomaly_config.h"
#include "sensor_data.h"

namespace {

struct Ctx {
  // calibration
  bool     calibrating = true;
  uint32_t calibStartMs = 0;
  uint32_t calibEndMs = 0;
  bool     vocSeeded = false;
  double   vocSum = 0;
  uint32_t vocN = 0;
  double   brSum = 0;
  uint32_t brN = 0;

  // baselines
  float baseVoc = 0;       // slowly adapting TVOC baseline (during exhalation)
  float baseBr = 0;        // calibrated breathing-rate baseline (0 = unavailable)

  // freshness tracking
  uint32_t lastSgTs = 0;
  uint32_t lastBrTs = 0;

  // TVOC flag
  bool     vocAbnormal = false;
  uint32_t lastExhaleMs = 0;
  float    dVoc = 0;

  // breathing-rate flag
  uint8_t  brCount = 0;
  bool     brAbnormal = false;

  uint32_t evalCount = 0;
} c;

void publish(SystemState s) {
  SystemStatus st = { s, c.vocAbnormal, c.brAbnormal };
  xQueueOverwrite(qStatus, &st);
}

void startCalibration() {
  uint32_t now = millis();
  c = Ctx();                       // reset everything
  c.calibrating = true;
  c.calibStartMs = now;
  publish(SystemState::CALIBRATING);
  Serial.println("[ANOMALY] CALIBRATION STARTED - wear the mask and breathe normally");
}

void evaluate() {
  uint32_t now = millis();
  c.evalCount++;

  Sgp30Data  sg;  bool haveSg = (xQueuePeek(qSgp30,  &sg, 0) == pdTRUE);
  Ld6002Data ld;  bool haveLd = (xQueuePeek(qLd6002, &ld, 0) == pdTRUE);

  bool newSg = haveSg && sg.timestampMs != c.lastSgTs;
  bool newBr = haveLd && ld.breathTimestampMs != 0 && ld.breathTimestampMs != c.lastBrTs;
  if (newSg) c.lastSgTs = sg.timestampMs;
  if (newBr) c.lastBrTs = ld.breathTimestampMs;

  // ================= CALIBRATION =================
  if (c.calibrating) {
    uint32_t elapsed = now - c.calibStartMs;

    if (newSg && elapsed > SGP30_WARMUP_MS) {
      c.vocSum += sg.tvoc;
      c.vocN++;
      if (sg.exhaling) {
        if (!c.vocSeeded) { c.baseVoc = sg.tvoc; c.vocSeeded = true; }
        else c.baseVoc = (1.0f - VOC_ALPHA_CALIB) * c.baseVoc + VOC_ALPHA_CALIB * sg.tvoc;
      }
    }
    if (newBr) { c.brSum += ld.breathRate; c.brN++; }

    if (elapsed >= CALIB_TIME_MS) {
      if (!c.vocSeeded && c.vocN > 0) c.baseVoc = (float)(c.vocSum / c.vocN);   // no exhalation seen
      c.baseBr = (c.brN >= BR_CALIB_MIN_SAMPLES) ? (float)(c.brSum / c.brN) : 0.0f;
      c.calibrating = false;
      c.calibEndMs = now;
      Serial.printf("[ANOMALY] CALIBRATION DONE  TVOC base=%.1f%s  BR base=%.1f%s\n",
                    c.baseVoc, c.vocSeeded ? "" : " (mean fallback)",
                    c.baseBr, c.baseBr > 0 ? "" : " (unavailable, absolute limit only)");
      publish(SystemState::NORMAL);
    } else {
      publish(SystemState::CALIBRATING);
      if (c.evalCount % 5 == 0) {
        Serial.printf("[ANOMALY] calibrating %lus/%lus  TVOC:%u eCO2:%u BR:%.1f\n",
                      (unsigned long)(elapsed / 1000), (unsigned long)(CALIB_TIME_MS / 1000),
                      haveSg ? sg.tvoc : 0, haveSg ? sg.eco2 : 0, haveLd ? ld.breathRate : 0.0f);
      }
    }
    return;
  }

  // ================= TVOC (exhalation only) =================
  if (newSg && sg.exhaling) {
    if (!c.vocAbnormal) {   // don't let a sustained rise be absorbed into the baseline
      c.baseVoc = (1.0f - VOC_ALPHA_RUN) * c.baseVoc + VOC_ALPHA_RUN * sg.tvoc;
    }
    c.dVoc = (float)sg.tvoc - c.baseVoc;
    c.vocAbnormal = (c.dVoc > VOC_SHIFT_THRESH_PPB);
    c.lastExhaleMs = now;
  }
  if (c.vocAbnormal && (now - c.lastExhaleMs) > VOC_HOLD_MS) c.vocAbnormal = false;

  // ================= BREATHING RATE =================
  bool brFresh = haveLd && ld.breathTimestampMs != 0 && (now - ld.breathTimestampMs) < BR_STALE_MS;
  bool brRaw = false;
  if (brFresh) {
    float br = ld.breathRate;
    brRaw = (br < BR_LOW_ABS_BPM);
    if (BR_CHECK_DROP && c.baseBr > 0.0f && br < c.baseBr * BR_DROP_RATIO) brRaw = true;
    if (BR_CHECK_HIGH && br > BR_HIGH_ABS_BPM) brRaw = true;
  } else if (BR_INVALID_IS_ABNORMAL) {
    // time since the last valid reading (or since calibration ended if none yet)
    uint32_t ref = (haveLd && ld.breathTimestampMs > c.calibEndMs) ? ld.breathTimestampMs : c.calibEndMs;
    brRaw = (now - ref) > BR_LOSS_TIMEOUT_MS;
  }
  c.brCount = brRaw ? (uint8_t)(c.brCount < 255 ? c.brCount + 1 : 255) : 0;
  c.brAbnormal = (c.brCount >= BR_CONFIRM_COUNT);

  // ================= FUSION =================
  SystemState s;
  if (c.vocAbnormal && c.brAbnormal)      s = SystemState::HIGH_ANOMALY;
  else if (c.vocAbnormal || c.brAbnormal) s = SystemState::LOW_ANOMALY;
  else                                    s = SystemState::NORMAL;
  publish(s);

  Serial.printf("[%s] BR:%.1f%s (base %.1f) HR:%.0f | TVOC:%u base:%.1f d:%.1f eCO2:%u exh:%d | vocAbn:%d brAbn:%d\n",
                stateName(s),
                haveLd ? ld.breathRate : 0.0f, brFresh ? "" : "(stale)", c.baseBr,
                haveLd ? ld.heartRate : 0.0f,
                haveSg ? sg.tvoc : 0, c.baseVoc, c.dVoc, haveSg ? sg.eco2 : 0,
                (haveSg && sg.exhaling) ? 1 : 0,
                c.vocAbnormal, c.brAbnormal);
}

void anomalyTask(void*) {
#if ENABLE_RECAL_BUTTON
  pinMode(PIN_RECAL_BTN, INPUT_PULLUP);
  bool lastBtn = HIGH;
  uint32_t btnMs = 0;
#endif

  startCalibration();
  uint32_t lastEval = millis();

  for (;;) {
#if ENABLE_RECAL_BUTTON
    bool b = digitalRead(PIN_RECAL_BTN);
    if (b == LOW && lastBtn == HIGH && millis() - btnMs > BUTTON_DEBOUNCE_MS) {
      startCalibration();
      btnMs = millis();
    }
    lastBtn = b;
#endif

    if (millis() - lastEval >= ANOMALY_EVAL_MS) {
      lastEval += ANOMALY_EVAL_MS;
      evaluate();
    }
    vTaskDelay(pdMS_TO_TICKS(ANOMALY_TICK_MS));
  }
}

} // namespace

void anomalyTaskStart() {
  xTaskCreatePinnedToCore(anomalyTask, "Anomaly", STACK_ANOMALY, nullptr, PRIO_ANOMALY, nullptr, APP_CORE);
}
