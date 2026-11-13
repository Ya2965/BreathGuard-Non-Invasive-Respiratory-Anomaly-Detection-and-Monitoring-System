/*
 * Alert task - LEDs + buzzer
 *
 *   CALIBRATING   : green blinks slowly, buzzer off
 *   NORMAL        : green solid,  buzzer off
 *   LOW ANOMALY   : yellow solid, slow beep
 *   HIGH ANOMALY  : red solid,    very fast beep
 */
#include "config.h"
#include "sensor_data.h"

namespace {

void setLeds(bool g, bool y, bool r) {
  digitalWrite(PIN_LED_GREEN,  g);
  digitalWrite(PIN_LED_YELLOW, y);
  digitalWrite(PIN_LED_RED,    r);
}

void alertTask(void*) {
  pinMode(PIN_LED_GREEN,  OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_RED,    OUTPUT);
  pinMode(PIN_BUZZER,     OUTPUT);

  // Power-on self test: all LEDs + short beep
  setLeds(true, true, true);
  digitalWrite(PIN_BUZZER, HIGH);
  vTaskDelay(pdMS_TO_TICKS(300));
  digitalWrite(PIN_BUZZER, LOW);
  vTaskDelay(pdMS_TO_TICKS(300));
  setLeds(false, false, false);

  SystemState current = SystemState::CALIBRATING;
  SystemState prev = SystemState::HIGH_ANOMALY;     // force first update
  bool greenBlink = false;
  bool buzzOn = false;
  uint32_t phaseStart = 0;

  for (;;) {
    SystemStatus st;
    if (xQueuePeek(qStatus, &st, 0) == pdTRUE) current = st.state;

    uint32_t now = millis();

    if (current != prev) {            // pattern changed: restart cleanly
      prev = current;
      phaseStart = now;
      greenBlink = false;
      buzzOn = false;
      digitalWrite(PIN_BUZZER, LOW);
      switch (current) {
        case SystemState::CALIBRATING:  setLeds(false, false, false); break;
        case SystemState::NORMAL:       setLeds(true,  false, false); break;
        case SystemState::LOW_ANOMALY:  setLeds(false, true,  false); break;
        case SystemState::HIGH_ANOMALY: setLeds(false, false, true);  break;
      }
    }

    switch (current) {
      case SystemState::CALIBRATING:
        if (now - phaseStart >= GREEN_CALIB_BLINK_MS) {
          greenBlink = !greenBlink;
          digitalWrite(PIN_LED_GREEN, greenBlink);
          phaseStart = now;
        }
        break;

      case SystemState::LOW_ANOMALY:
        if (now - phaseStart >= (buzzOn ? BEEP_LOW_ON_MS : BEEP_LOW_OFF_MS)) {
          buzzOn = !buzzOn;
          digitalWrite(PIN_BUZZER, buzzOn);
          phaseStart = now;
        }
        break;

      case SystemState::HIGH_ANOMALY:
        if (now - phaseStart >= (buzzOn ? BEEP_HIGH_ON_MS : BEEP_HIGH_OFF_MS)) {
          buzzOn = !buzzOn;
          digitalWrite(PIN_BUZZER, buzzOn);
          phaseStart = now;
        }
        break;

      case SystemState::NORMAL:
      default:
        break;
    }

    vTaskDelay(pdMS_TO_TICKS(ALERT_TICK_MS));
  }
}

} // namespace

void alertTaskStart() {
  xTaskCreatePinnedToCore(alertTask, "Alert", STACK_ALERT, nullptr, PRIO_ALERT, nullptr, APP_CORE);
}
