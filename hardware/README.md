# Hardware

Component-level description of the BreathGuard prototype. Schematics go in `schematics/`; pin-level wiring is in `../connections/`.

## Components

| Component | Purpose | Interface |
|---|---|---|
| ESP32 DevKit | Main controller, runs FreeRTOS firmware | – |
| HLK-LD6002 (60 GHz FMCW radar) | Contactless breathing-rate (and heart-rate) measurement | UART2, 1382400 baud |
| SGP30 gas sensor | TVOC + eCO₂ from exhaled breath | I2C |
| Smart Mask | Enclosure that guides exhaled air over the SGP30 | – |
| Green / Yellow / Red LED | Normal / Low anomaly / High anomaly | GPIO |
| Active buzzer | Acoustic alert (slow beep / very fast beep) | GPIO |
| Push button *(optional)* | Restart calibration | GPIO |
| Resistors 220–330 Ω ×3 | LED current limiting | – |

## LD6002 radar

- 60 GHz FMCW radar module for contactless respiration and heartbeat measurement.
- Public specification: respiration approx. 9–48 breaths/min, heartbeat approx. 60–150 bpm; detection distance up to about **1.5 m**.
- Place it facing the user's chest/torso, fixed and stable (e.g. on a stand beside the bed). Movement of the person or of the radar itself disturbs the measurement; avoid fans and other moving objects in the beam.
- Only **breathing rate** is used for anomaly detection. Heart rate is read and printed on the serial log for reference.
- Check the datasheet of your exact module revision for supply voltage, baud rate and frame format.

## SGP30 and Smart Mask

- The SGP30 is mounted inside the mask so that exhaled air passes over it; **eCO₂ rising** marks an exhalation and TVOC is evaluated only during these periods, which reduces the influence of ambient VOCs.
- The SGP30 `eCO₂` is a value **estimated** from its hydrogen signal, not a true CO₂ measurement, and the sensor responds slowly. The sensor has a ~15 s warm-up after power-up.
- Keep the sensor away from alcohol-based cleaners, perfumes and hand sanitiser before and during measurements – they cause large TVOC spikes.
- Hygiene: use a barrier/cover appropriate for skin-adjacent use; the SGP30 is not designed for direct contact with moisture or saliva.

## Alert hardware

| State | LED | Buzzer |
|---|---|---|
| Calibrating | Green blinking | Off |
| Normal | Green solid | Off |
| Low-level anomaly | Yellow | Slow beep (200 ms on / 800 ms off) |
| High-level anomaly | Red | Very fast beep (100 ms on / 100 ms off) |

## Schematics

Put exported schematic files (KiCad / Fritzing / PDF / PNG) in `schematics/`.
