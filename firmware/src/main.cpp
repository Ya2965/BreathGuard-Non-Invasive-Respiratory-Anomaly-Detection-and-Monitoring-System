/*
 * BreathGuard - Non-Invasive Respiratory Anomaly Detection and Monitoring System
 * ESP32 + FreeRTOS
 *
 *   sgp30_task    : SGP30 in Smart Mask -> TVOC, eCO2, exhalation flag
 *   ld6002_task   : LD6002 mmWave radar -> breathing rate (UART frames)
 *   anomaly_task  : calibration, baselines, data fusion -> NORMAL / LOW / HIGH
 *   alert_task    : green / yellow / red LED + buzzer patterns
 *
 * Experimental prototype. Not a medical device.
 */
#include <Arduino.h>
#include "config.h"
#include "sensor_data.h"

QueueHandle_t qSgp30  = nullptr;
QueueHandle_t qLd6002 = nullptr;
QueueHandle_t qStatus = nullptr;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("BreathGuard booting (FreeRTOS)");

  qSgp30  = xQueueCreate(1, sizeof(Sgp30Data));
  qLd6002 = xQueueCreate(1, sizeof(Ld6002Data));
  qStatus = xQueueCreate(1, sizeof(SystemStatus));

  if (!qSgp30 || !qLd6002 || !qStatus) {
    Serial.println("FATAL: queue creation failed");
    while (true) delay(1000);
  }

  // Initial status so the alert task has something to show immediately
  SystemStatus init = { SystemState::CALIBRATING, false, false };
  xQueueOverwrite(qStatus, &init);

  alertTaskStart();     // LEDs first: gives visible feedback during start-up
  ld6002TaskStart();
  sgp30TaskStart();
  anomalyTaskStart();

  Serial.println("Tasks started");
}

void loop() {
  // All work is done in FreeRTOS tasks.
  vTaskDelay(portMAX_DELAY);
}
