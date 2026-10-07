# BreathGuard

### Non-Invasive Respiratory Anomaly Detection and Monitoring System

BreathGuard is a **non-invasive respiratory monitoring system** designed to monitor individuals with **sleep apnea and other respiratory conditions** and identify potentially abnormal respiratory patterns in real time.

The system combines **contactless breathing-rate monitoring** using an **LD6002 mmWave radar** with **exhaled-breath VOC sensing** using an **SGP30 gas sensor** integrated into a **Smart Mask**.

An **ESP32** acts as the central controller, while **FreeRTOS** is used to manage real-time sensor acquisition, processing, anomaly detection, and alert generation.

The system focuses on two primary respiratory indicators:

- **Decrease in breathing rate**
- **Increase in exhaled-breath TVOC**

The SGP30 is placed inside the Smart Mask, where **eCO₂ is used as an indicator of exhalation**. TVOC measurements are considered during detected exhalation periods, helping reduce the influence of VOCs from the surrounding environment.

The two respiratory indicators are then combined to classify the current state as **Normal, Low-Level Anomaly, or High-Level Anomaly**.

---

## Intended Application

BreathGuard is intended as an **experimental respiratory monitoring and early-warning system** for individuals who may require respiratory observation.

Potential applications include monitoring individuals with:

- **Sleep apnea**
- Other respiratory disorders
- Abnormal or irregular breathing patterns
- Conditions requiring continuous or periodic respiratory monitoring

The system is designed to provide **non-invasive monitoring** and generate an alert when potentially abnormal respiratory patterns are detected.

BreathGuard is intended to function as a **monitoring and anomaly-alert system**, rather than a diagnostic system.

---

## Key Features

- **Non-invasive respiratory monitoring**
- **Contactless breathing-rate measurement** using LD6002 mmWave radar
- **Exhaled-breath VOC monitoring** using SGP30
- **Smart Mask** for controlled breath sensing
- **eCO₂-based exhalation detection**
- **Two-level anomaly detection**
- **Real-time processing using ESP32 and FreeRTOS**
- **Visual anomaly indication using LEDs**
- **Different acoustic alert patterns using a buzzer**
- Designed for monitoring individuals requiring respiratory observation

---

## System Overview

BreathGuard monitors two complementary respiratory indicators:

1. **Breathing rate**
2. **Changes in TVOC during exhalation**

The **LD6002 mmWave radar** monitors respiratory motion without physical contact with the user and provides breathing-rate information.

The **SGP30 gas sensor** is placed inside the Smart Mask and measures TVOC and eCO₂ from the user's exhaled breath.

Rather than continuously treating every TVOC fluctuation as a respiratory event, the system uses an increase in **eCO₂ as an indicator of exhalation**. TVOC measurements are considered primarily during detected exhalation periods.

The ESP32 combines the breathing-rate information with the breath-related TVOC information to determine the current respiratory state.

```text
                         BREATHGUARD
                              │
              ┌───────────────┴───────────────┐
              │                               │
       SMART MASK                         LD6002 RADAR
              │                               │
            SGP30                    Respiratory Motion
              │                               │
       TVOC + eCO₂                    Breathing Rate
              │                               │
       eCO₂ increasing?                     │
              │                             │
              ↓                             │
      Exhalation detected                   │
              │                             │
              ↓                             │
       Consider TVOC                        │
              │                             │
              └──────────────┬──────────────┘
                             │
                             ↓
                       ESP32 + FreeRTOS
                             │
                             ↓
                     Anomaly Detection
                             │
                    ┌────────┴────────┐
                    │                 │
                 NORMAL            ANOMALY
                    │                 │
              Green LED        ┌──────┴──────┐
              Buzzer OFF       │             │
                           LOW ANOMALY   HIGH ANOMALY
                               │             │
                         One parameter   Both parameters
                           abnormal        abnormal
                               │             │
                          Yellow LED      Red LED
                          Slow Beep     Very Fast Beep
```

---

## Smart Mask

The SGP30 is integrated into a dedicated mask enclosure referred to as the **Smart Mask**.

The Smart Mask provides a controlled environment for sensing the user's exhaled breath.

The SGP30 provides:

- **TVOC** — Total Volatile Organic Compounds
- **eCO₂** — equivalent CO₂

The eCO₂ signal is used to identify periods where the user is exhaling.

```text
                   SMART MASK
              ┌─────────────────┐
              │                 │
  Exhaled ──→ │      SGP30      │
   Breath     │                 │
              │   TVOC + eCO₂   │
              └────────┬────────┘
                       │
                  eCO₂ increase
                       │
                       ↓
              Exhalation detected
                       │
                       ↓
                TVOC considered
                       │
                       ↓
              Anomaly processing
```

This approach helps reduce the influence of **external environmental VOC fluctuations** on the respiratory monitoring system.

The mask acts as a **non-invasive interface for breath sensing** and does not require invasive physiological measurements.

---

## Respiratory Monitoring

### Contactless Breathing-Rate Monitoring

The LD6002 mmWave radar is used to detect small respiratory movements without requiring physical contact with the user's body.

```text
          User
           │
           │ Respiratory motion
           ↓
      ┌───────────┐
      │  LD6002   │
      │   Radar   │
      └─────┬─────┘
            │
            ↓
      Respiratory signal
            │
            ↓
       Breathing Rate
```

This provides a **contactless method of monitoring respiratory rate** without requiring:

- Chest straps
- Skin electrodes
- Body-mounted motion sensors for respiratory-rate measurement

---

## Anomaly Detection

BreathGuard has **two levels of anomaly detection** in addition to the normal operating state.

### System States

| State | Condition |
|---|---|
| **Normal** | Neither parameter is abnormal |
| **Low-Level Anomaly** | Only one parameter is abnormal |
| **High-Level Anomaly** | Both parameters are abnormal |

---

### Normal

Neither respiratory parameter indicates an anomaly.

```text
TVOC            → Normal
Breathing Rate  → Normal
```

**Indication:**

- 🟢 Green LED
- Buzzer OFF

---

### Low-Level Anomaly

A low-level anomaly occurs when **only one** of the two monitored parameters becomes abnormal.

```text
TVOC increase
      OR
Breathing-rate decrease
```

For example:

```text
TVOC abnormal
Breathing rate normal
        ↓
   LOW ANOMALY
```

or:

```text
TVOC normal
Breathing rate abnormal
        ↓
   LOW ANOMALY
```

**Indication:**

- 🟡 Yellow LED
- Slow buzzer pattern

A single abnormal parameter is therefore treated as a lower-level warning.

---

### High-Level Anomaly

A high-level anomaly occurs when **both respiratory indicators become abnormal simultaneously**.

```text
TVOC increase
      AND
Breathing-rate decrease
```

Conceptually:

```text
         TVOC ↑
            │
            ├────── AND ──────→ HIGH ANOMALY
            │
     Breathing Rate ↓
```

**Indication:**

- 🔴 Red LED
- Very fast buzzer pattern

The simultaneous occurrence of both conditions represents a stronger anomaly condition than either parameter alone.

---

## Alert System

BreathGuard provides both visual and acoustic feedback.

| System State | LED | Buzzer |
|---|---|---|
| Normal | 🟢 Green | OFF |
| Low-Level Anomaly | 🟡 Yellow | Slow beep |
| High-Level Anomaly | 🔴 Red | Very fast beep |

The buzzer pattern is intentionally different for each anomaly level so that the severity can be identified without continuously looking at the device.

```text
NORMAL

Green LED
Buzzer OFF


LOW-LEVEL ANOMALY

Yellow LED
Beep ──── Beep ──── Beep


HIGH-LEVEL ANOMALY

Red LED
Beep-Beep-Beep-Beep-Beep
```

---

## Software Architecture

The firmware is built using **FreeRTOS** on the ESP32.

Different functions of the system are separated into independent RTOS tasks.

```text
┌───────────────────────────────────┐
│               ESP32               │
│                                   │
│  ┌─────────────────────────────┐  │
│  │         SGP30 Task          │  │
│  │      TVOC + eCO₂ data       │  │
│  └──────────────┬──────────────┘  │
│                 │                 │
│  ┌──────────────▼──────────────┐  │
│  │        LD6002 Task           │  │
│  │       Breathing Rate         │  │
│  └──────────────┬──────────────┘  │
│                 │                 │
│  ┌──────────────▼──────────────┐  │
│  │    Anomaly Detection Task    │  │
│  │       Sensor Data Fusion     │  │
│  └──────────────┬──────────────┘  │
│                 │                 │
│  ┌──────────────▼──────────────┐  │
│  │          Alert Task          │  │
│  │        LEDs + Buzzer         │  │
│  └─────────────────────────────┘  │
│                                   │
└───────────────────────────────────┘
```

The FreeRTOS implementation separates:

- Sensor acquisition
- Respiratory parameter processing
- Exhalation detection
- Anomaly detection
- Alert generation

FreeRTOS mechanisms such as **tasks, delays, queues, shared data and task synchronization** are used to coordinate these functions in real time.

---

## Hardware

| Component | Purpose |
|---|---|
| **ESP32** | Main controller and real-time processing |
| **LD6002 mmWave Radar** | Contactless breathing-rate monitoring |
| **SGP30** | TVOC and eCO₂ sensing |
| **Smart Mask** | Controlled environment for exhaled-breath sensing |
| **Green LED** | Normal system state |
| **Yellow LED** | Low-level anomaly |
| **Red LED** | High-level anomaly |
| **Buzzer** | Acoustic anomaly indication |

---

## System Flow

The overall operation can be summarized as:

```text
                    START
                      │
                      ↓
               Initialize ESP32
                      │
                      ↓
              Initialize Sensors
                      │
          ┌───────────┴───────────┐
          │                       │
          ↓                       ↓
       SGP30                   LD6002
          │                       │
     TVOC + eCO₂            Respiratory Motion
          │                       │
          ↓                       ↓
    eCO₂ rising?            Breathing Rate
          │                       │
     ┌────┴────┐                  │
     │         │                  │
    YES        NO                 │
     │         │                  │
     ↓         └───────┐          │
 Consider TVOC         │          │
     │                 │          │
     └────────┬────────┘          │
              │                   │
              └─────────┬─────────┘
                        ↓
                 Anomaly Detection
                        │
             ┌──────────┼──────────┐
             ↓          ↓          ↓
          NORMAL       LOW        HIGH
             │          │          │
             ↓          ↓          ↓
          Green      Yellow       Red
           LED         LED         LED
             │          │          │
          No beep    Slow beep   Fast beep
```

---

## Hardware Architecture

```text
                     ┌─────────────────┐
                     │      ESP32      │
                     │                 │
                     │    FreeRTOS     │
                     │    Processing   │
                     └───────┬─────────┘
                             │
             ┌───────────────┼───────────────┐
             │               │               │
             ↓               ↓               ↓
        ┌─────────┐     ┌──────────┐    ┌─────────┐
        │  SGP30  │     │  LD6002  │    │  Alert  │
        │         │     │          │    │ System  │
        └─────────┘     └──────────┘    └────┬────┘
             │               │                │
             │               │          ┌─────┴─────┐
             │               │          │           │
             │               │         LEDs       Buzzer
             │               │
        Smart Mask       Contactless
        Breath Data      Respiration
```

---

## Repository Structure

```text
BreathGuard/
│
├── README.md
│
├── firmware/
│   ├── src/
│   │   ├── main.cpp
│   │   ├── sgp30_task.cpp
│   │   ├── ld6002_task.cpp
│   │   ├── anomaly_task.cpp
│   │   └── alert_task.cpp
│   │
│   ├── include/
│   │   ├── config.h
│   │   ├── sensor_data.h
│   │   └── anomaly_config.h
│   │
│   └── README.md
│
├── hardware/
│   ├── schematics/
│   └── README.md
│
├── connections/
│   ├── pinout.md
│   └── README.md
│
└── references/
    ├── datasheets/
    ├── papers/
    └── notes/
```

### `firmware/`

Contains the ESP32 firmware and FreeRTOS implementation.

This directory contains:

- Sensor acquisition tasks
- SGP30 processing
- LD6002 processing
- Exhalation detection
- Anomaly detection
- LED control
- Buzzer control
- System configuration

---

### `hardware/`

Contains hardware design files such as:

- Circuit schematics
- Hardware architecture
- Component-level documentation
- Physical Smart Mask design information

---

### `connections/`

Contains practical connection information between the components.

This includes:

- ESP32 pin assignments
- SGP30 connections
- LD6002 UART connections
- LED connections
- Buzzer connection
- Power connections
- Wiring diagrams

---

### `references/`

Contains technical material used during development.

This may include:

- SGP30 datasheet
- LD6002 documentation
- ESP32 documentation
- FreeRTOS references
- Relevant respiratory monitoring research papers
- Sensor and signal-processing notes

---

## Technology Stack

### Hardware

- ESP32
- SGP30
- LD6002 mmWave radar
- LEDs
- Buzzer
- Smart Mask

### Software

- C/C++
- FreeRTOS
- ESP32 Arduino / ESP-IDF environment
- UART communication
- I²C communication

---

## Non-Invasive Monitoring Approach

BreathGuard is designed around a **non-invasive sensing approach**.

The system does not require:

- Needles or blood sampling
- Skin electrodes
- Sensors attached directly to the body for respiratory-rate measurement
- Surgical or internal sensors

The **LD6002 provides contactless respiratory monitoring**, while the Smart Mask provides a simple, non-invasive interface for collecting exhaled-breath information.

Therefore, the system combines **contactless physiological sensing** with **non-invasive breath sensing** in a wearable monitoring setup.

---

## Disclaimer

BreathGuard is an **experimental embedded-system prototype** for respiratory monitoring and anomaly detection.

It is **not a medical device** and is not intended to provide a medical diagnosis, replace professional medical equipment, or be used as the sole basis for medical decisions.

---

## Author

**Yash Jadhav**

Automation & Robotics Engineering  
VESIT, Mumbai
