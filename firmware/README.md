# Firmware

ESP32 firmware for BreathGuard, written for the Arduino framework with **FreeRTOS** tasks (FreeRTOS is built into the ESP32 core).

```
firmware/
├── platformio.ini            # PlatformIO build file for this layout
├── include/
│   ├── config.h              # pins, UART/I2C, timing, RTOS stack/priority
│   ├── anomaly_config.h      # calibration + detection thresholds
│   └── sensor_data.h         # shared structs, system states, queue handles
└── src/
    ├── main.cpp              # creates queues, starts tasks
    ├── sgp30_task.cpp        # SGP30: TVOC, eCO₂, exhalation detection
    ├── ld6002_task.cpp       # LD6002: UART frame parser -> breathing rate
    ├── anomaly_task.cpp      # calibration, baselines, data fusion
    └── alert_task.cpp        # green / yellow / red LED + buzzer
```

## Build and upload

**PlatformIO (recommended for this layout)**

```bash
cd firmware
pio run -t upload
pio device monitor        # 115200 baud
```

**Arduino IDE:** it does not use the `src/` + `include/` layout directly. Create a sketch folder, rename `main.cpp` to `<folder>.ino`, copy all other `.cpp` and `.h` files into the same folder, install **Adafruit SGP30 Sensor**, select *ESP32 Dev Module* and upload.

## Tasks

| Task | Priority | Period | Role |
|---|---|---|---|
| `LD6002` | 4 | polls UART every 5 ms | Parses radar frames (checksum-verified), publishes breathing/heart rate |
| `SGP30` | 3 | 1 s (`vTaskDelayUntil`) | Reads TVOC/eCO₂, sets `exhaling` when eCO₂ keeps rising |
| `Anomaly` | 2 | 1 s evaluation, 50 ms tick | Calibration, baselines, fusion → NORMAL / LOW / HIGH |
| `Alert` | 2 | 10 ms | LED and buzzer patterns |

Data flows through length-1 queues used as mailboxes (`xQueueOverwrite` / `xQueuePeek`):

```
SGP30 task ──qSgp30──┐
                     ├─► Anomaly task ──qStatus──► Alert task
LD6002 task ─qLd6002─┘
```

## Detection logic

| Parameter | Abnormal when |
|---|---|
| **TVOC** | During an exhalation, TVOC is more than 80 ppb above the calibrated baseline (flag held up to 15 s without exhalation) |
| **Breathing rate** | Rate < 12 bpm, **or** < 70 % of the calibrated baseline, **or** no valid radar reading for > 15 s; must persist 3 evaluations |

| TVOC | Breathing rate | State | Indication |
|---|---|---|---|
| normal | normal | **Normal** | Green, buzzer off |
| abnormal | normal | **Low anomaly** | Yellow, slow beep |
| normal | abnormal | **Low anomaly** | Yellow, slow beep |
| abnormal | abnormal | **High anomaly** | Red, very fast beep |

### Calibration

At boot (and when the optional button is pressed) the firmware calibrates for 60 s while the green LED blinks. The wearer should have the mask on and breathe normally in front of the radar. It learns the TVOC baseline (from exhalation samples, ignoring the SGP30's first 15 s) and the mean breathing rate. After calibration the TVOC baseline adapts slowly (α = 0.02) and does **not** adapt while TVOC is flagged abnormal, so a sustained rise cannot silently become the new "normal".

## LD6002 interface

UART2, GPIO16 (RX) / GPIO17 (TX), 8N1, **1382400 baud** by default (115200 is tried automatically if no valid frame arrives within 5 s).

| Field | Bytes |
|---|---|
| Start | `0x01` |
| ID | 2 (ignored) |
| Length | 2, big-endian |
| Type | 2, big-endian (`0x0A14` breathing rate, `0x0A15` heart rate) |
| Header checksum | `~XOR(bytes 0..6)` |
| Payload | `Length` bytes (float32 for the rates) |
| Data checksum | `~XOR(payload)` |

The layout is taken from public HLK-LD6002 libraries. Because radar firmware versions differ, confirm on your module: set `LD6002_DEBUG` to `1` in `config.h` and check that frames of type `0x0A14` appear and that values look plausible (a rate of 0 means "no valid measurement" and is ignored).

## Serial log (115200 baud)

```
[ANOMALY] CALIBRATION STARTED - wear the mask and breathe normally
[ANOMALY] CALIBRATION DONE  TVOC base=21.4  BR base=15.8
[NORMAL] BR:16.0 (base 15.8) HR:72 | TVOC:22 base:21.4 d:0.6 eCO2:540 exh:1 | vocAbn:0 brAbn:0
[LOW ANOMALY] BR:8.0 (base 15.8) HR:70 | TVOC:23 base:21.4 d:1.6 eCO2:520 exh:1 | vocAbn:0 brAbn:1
[HIGH ANOMALY] BR:8.0 (base 15.8) HR:70 | TVOC:190 base:21.4 d:168.6 eCO2:760 exh:1 | vocAbn:1 brAbn:1
```

## Bench testing without hardware

In `config.h`, set `SIMULATE_SGP30` and/or `SIMULATE_LD6002` to `1` to generate random data. **Set them back to 0 for the real device.**

## Tuning

All thresholds are in `include/anomaly_config.h`. The defaults are starting values for a prototype, not clinically validated limits – tune them on your hardware and test subject. A change in sensor placement (radar distance/angle, mask fit) usually requires recalibration.

## Limitations

- Experimental prototype; **not a medical device**.
- SGP30 `eCO₂` is an estimate and the sensor is slow; exhalation detection is a heuristic.
- LD6002 accuracy drops with body movement, distance and orientation.
- "No valid radar reading" is treated as a breathing anomaly (can be a lost target rather than real apnea). Set `BR_INVALID_IS_ABNORMAL` to `false` to disable.
