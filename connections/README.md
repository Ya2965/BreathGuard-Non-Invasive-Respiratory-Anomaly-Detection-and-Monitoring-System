# Connections

How the BreathGuard components are wired to the ESP32.

| File | Content |
|---|---|
| `pinout.md` | GPIO assignment table (source of truth: `firmware/include/config.h`) |
| `wiring_diagram.png` | Wiring overview |

## Connection overview

```text
                        ┌──────────────┐
  SGP30 (Smart Mask) ───┤ I2C 21/22    │
  LD6002 radar ─────────┤ UART2 16/17  │
                        │    ESP32     ├── GPIO23 ─ R ─ Green LED ──┐
  Recalibrate button ───┤ GPIO5        ├── GPIO25 ─ R ─ Yellow LED ─┤
  (optional)            │              ├── GPIO18 ─ R ─ Red LED ────┼─ GND
                        │              ├── GPIO19 ─── Buzzer (+) ───┘
                        └──────────────┘
```

## 1. SGP30 (inside the Smart Mask) – I2C

| SGP30 | ESP32 |
|---|---|
| VIN | 3V3 |
| GND | GND |
| SDA | GPIO21 |
| SCL | GPIO22 |

- I2C address is `0x58`. The Adafruit breakout already has pull-ups.
- Keep the wires from the mask to the ESP32 short, or use shielded/twisted wires (I2C is sensitive over long leads).
- The SGP30 reports `400 ppm / 0 ppb` for ~15 s after power-up; the firmware ignores this period.

## 2. LD6002 radar – UART

| LD6002 | ESP32 |
|---|---|
| 3V3 (power) | 3V3 or separate 3.3 V regulator |
| GND | GND |
| **TX** | **GPIO16 (RX2)** |
| RX *(optional)* | GPIO17 (TX2) |

- **TX ↔ RX are crossed**: radar TX goes to ESP32 RX.
- Default radar baud rate is **1382400**, 8N1. Some firmware versions use 115200; the firmware automatically tries both.
- Mount the radar facing the user's chest, within about 1.5 m (see `hardware/README.md`).

## 3. Alert outputs

| Output | ESP32 | Part |
|---|---|---|
| Green LED | GPIO23 → 220–330 Ω → LED anode | cathode → GND |
| Yellow LED | GPIO25 → 220–330 Ω → LED anode | cathode → GND |
| Red LED | GPIO18 → 220–330 Ω → LED anode | cathode → GND |
| Buzzer | GPIO19 → buzzer (+) | buzzer (−) → GND |

- Use an **active** buzzer (it beeps when powered, no tone generation needed).
- If the buzzer draws more than ~20 mA, drive it through an NPN transistor (e.g. 2N2222 with 1 kΩ base resistor) and add a flyback diode if it is an inductive type.

## 4. Optional recalibration button

Button between **GPIO5** and **GND**. The internal pull-up is enabled in firmware, so no resistor is needed. Disable by setting `ENABLE_RECAL_BUTTON` to `0` in `config.h`.

## Checklist before power-on

- [ ] Common GND between ESP32, SGP30, LD6002, LEDs and buzzer
- [ ] LD6002 TX → ESP32 GPIO16 (not GPIO17)
- [ ] LD6002 supplied with 3.3 V (not 5 V) unless your module's datasheet says otherwise
- [ ] Resistors in series with every LED
- [ ] No strapping pins used (see `pinout.md`)
