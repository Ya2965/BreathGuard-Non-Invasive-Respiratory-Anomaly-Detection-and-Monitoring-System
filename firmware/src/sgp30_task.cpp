/*
 * SGP30 task (Smart Mask)
 * Reads TVOC + eCO2 at 1 Hz and flags exhalation when eCO2 keeps rising.
 */
#include <Wire.h>
#include "Adafruit_SGP30.h"
#include "config.h"
#include "anomaly_config.h"
#include "sensor_data.h"

static void sgp30Task(void*) {
  Adafruit_SGP30 sgp;

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  bool ok = sgp.begin();
  while (!ok && !SIMULATE_SGP30) {
    Serial.println("[SGP30] not found, retrying...");
    vTaskDelay(pdMS_TO_TICKS(2000));
    ok = sgp.begin();
  }
  Serial.println(ok ? "[SGP30] OK" : "[SGP30] simulated");

  uint16_t prevCo2 = 400;
  uint8_t  riseCount = 0;
  uint8_t  failCount = 0;
  TickType_t last = xTaskGetTickCount();

  for (;;) {
    uint16_t tvoc = 0, eco2 = 400;
    bool got = false;

    if (ok && sgp.IAQmeasure()) {
      tvoc = sgp.TVOC;
      eco2 = sgp.eCO2;
      got = true;
      failCount = 0;
    } else if (ok) {
      if (++failCount == 5) Serial.println("[SGP30] read failures");
    }
#if SIMULATE_SGP30
    if (!got) {
      tvoc = random(0, 300);
      eco2 = random(400, 800);
      got = true;
    }
#endif

    if (got) {
      // Exhalation = eCO2 above minimum AND rising for N consecutive samples
      if (eco2 > prevCo2) riseCount++; else riseCount = 0;
      bool exhaling = (eco2 > ECO2_EXHALE_MIN_PPM && riseCount >= ECO2_RISE_COUNT);
      prevCo2 = eco2;

      Sgp30Data d = { tvoc, eco2, exhaling, (uint32_t)millis() };
      xQueueOverwrite(qSgp30, &d);
    }

    vTaskDelayUntil(&last, pdMS_TO_TICKS(SGP30_PERIOD_MS));
  }
}

void sgp30TaskStart() {
  xTaskCreatePinnedToCore(sgp30Task, "SGP30", STACK_SGP30, nullptr, PRIO_SGP30, nullptr, APP_CORE);
}
