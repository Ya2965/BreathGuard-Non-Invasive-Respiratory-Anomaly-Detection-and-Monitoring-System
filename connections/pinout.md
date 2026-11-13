# BreathGuard – ESP32 Pinout

Board: ESP32 DevKit (30/38-pin). All numbers are **GPIO numbers** and are defined in `firmware/include/config.h` – change them there and here together.

## Summary

| Function | ESP32 GPIO | Connects to | Direction | Notes |
|---|---|---|---|---|
| I2C SDA | **21** | SGP30 SDA | bidirectional | default I2C bus |
| I2C SCL | **22** | SGP30 SCL | output | default I2C bus |
| UART2 RX | **16** | LD6002 **TX** | input | radar data (1382400 baud, 8N1) |
| UART2 TX | **17** | LD6002 **RX** | output | optional, only needed to configure radar |
| Green LED | **23** | LED anode via resistor | output | NORMAL / calibrating (blink) |
| Yellow LED | **25** | LED anode via resistor | output | LOW-level anomaly |
| Red LED | **18** | LED anode via resistor | output | HIGH-level anomaly |
| Buzzer | **19** | active buzzer (+) | output | on/off, pattern set by software |
| Recalibrate button *(optional)* | **5** | button → GND | input, pull-up | restarts 60 s calibration |

## Power

| Rail | Used by |
|---|---|
| 3V3 | SGP30 VIN (3–5 V accepted by the Adafruit breakout), LD6002 (see note) |
| GND | **common ground for every module** |
| 5V / VIN | ESP32 supply (USB) |

> **LD6002 supply:** Hi-Link recommends a stable 3.3 V supply (≥ 1 A capable, low ripple) for the module. If the ESP32 3V3 pin sags or the radar resets, power it from a separate 3.3 V regulator and connect only the **grounds** together. Verify against the datasheet of the exact module revision you have.

## Pins to avoid on ESP32

GPIO 0, 2, 12, 15 (boot strapping), GPIO 6–11 (flash), GPIO 34–39 (input only – cannot drive LEDs/buzzer).

## Changing a pin

1. Edit the matching `#define PIN_...` in `firmware/include/config.h`.
2. Update the table above and `wiring_diagram.png`.
