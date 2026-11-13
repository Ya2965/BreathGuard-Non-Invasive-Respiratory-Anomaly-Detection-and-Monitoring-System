#pragma once
/*
 * BreathGuard - hardware / RTOS configuration
 * Pin numbers must match connections/pinout.md
 */
#include <Arduino.h>

// ======================= GPIO =======================
// I2C -> SGP30 (inside Smart Mask)
#define PIN_I2C_SDA        21
#define PIN_I2C_SCL        22

// UART2 -> LD6002 mmWave radar
#define PIN_LD6002_RX      16   // ESP32 RX  <- LD6002 TX
#define PIN_LD6002_TX      17   // ESP32 TX  -> LD6002 RX (optional, config only)

// Alert outputs
#define PIN_LED_GREEN      23   // NORMAL
#define PIN_LED_YELLOW     25   // LOW-LEVEL ANOMALY
#define PIN_LED_RED        18   // HIGH-LEVEL ANOMALY
#define PIN_BUZZER         19   // active buzzer, plain on/off

// Optional push button (to GND, internal pull-up): restart calibration
#define ENABLE_RECAL_BUTTON 1
#define PIN_RECAL_BTN      5
#define BUTTON_DEBOUNCE_MS 200

// ======================= LD6002 UART =======================
// HLK-LD6002 default is 1382400 baud, 8N1. Some firmware versions use 115200.
// The driver probes both if no valid frame is received.
#define LD6002_BAUD_PRIMARY    1382400UL
#define LD6002_BAUD_ALTERNATE  115200UL
#define LD6002_RX_BUFFER_BYTES 2048
#define LD6002_MAX_PAYLOAD     64      // larger frames (e.g. point clouds) are skipped
#define LD6002_BAUD_PROBE_MS   5000    // switch baud if no valid frame for this long
#define LD6002_DEBUG           0       // 1 = print every valid frame type/length

// ======================= BENCH TEST (no hardware) =======================
// 1 = generate fake data when the real sensor is not available.
// MUST be 0 on the real device.
#define SIMULATE_SGP30     0
#define SIMULATE_LD6002    0

// ======================= TIMING =======================
#define SGP30_PERIOD_MS    1000    // SGP30 requires IAQmeasure() at 1 Hz
#define LD6002_POLL_MS     5       // UART drain interval
#define ANOMALY_TICK_MS    50      // button polling + scheduler tick
#define ANOMALY_EVAL_MS    1000    // data fusion period
#define ALERT_TICK_MS      10      // LED / buzzer pattern resolution

// ======================= ALERT PATTERNS =======================
#define GREEN_CALIB_BLINK_MS 800   // green blinks while calibrating
#define BEEP_LOW_ON_MS       200   // low anomaly: slow beep
#define BEEP_LOW_OFF_MS      800
#define BEEP_HIGH_ON_MS      100   // high anomaly: very fast beep
#define BEEP_HIGH_OFF_MS     100

// ======================= RTOS =======================
#define STACK_SGP30        4096
#define STACK_LD6002       4096
#define STACK_ANOMALY      6144    // printf with floats needs headroom
#define STACK_ALERT        2048

#define PRIO_LD6002        4       // highest: must drain the fast UART
#define PRIO_SGP30         3
#define PRIO_ANOMALY       2
#define PRIO_ALERT         2

#define APP_CORE           1
