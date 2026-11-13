/*
 * LD6002 task - HLK-LD6002 60 GHz mmWave radar over UART2
 *
 * Frame format (as used by the public HLK-LD6002 libraries):
 *   [0]    0x01              start of frame
 *   [1-2]  ID                (ignored)
 *   [3-4]  LEN               payload length, big-endian
 *   [5-6]  TYPE              big-endian
 *   [7]    HEAD_CKSUM        ~XOR(bytes 0..6)
 *   [8..]  PAYLOAD           LEN bytes
 *   [last] DATA_CKSUM        ~XOR(payload)
 *
 * TYPE 0x0A14 = breathing rate (float32 bpm)
 * TYPE 0x0A15 = heart rate     (float32 bpm)
 * Other types (phase data 0x0A13, etc.) are verified and ignored.
 *
 * NOTE: float byte order, baud rate and the exact payload layout depend on
 * the radar firmware version. Set LD6002_DEBUG 1 to verify on your module.
 */
#include <math.h>
#include <string.h>
#include "config.h"
#include "sensor_data.h"

namespace {

constexpr uint8_t  SOF            = 0x01;
constexpr uint16_t TYPE_BREATH    = 0x0A14;
constexpr uint16_t TYPE_HEART     = 0x0A15;

enum class Rx : uint8_t { SOF, HEAD, PAYLOAD, PAYLOAD_CK, SKIP };

Rx       st = Rx::SOF;
uint8_t  head[8];
uint8_t  headIdx = 0;
uint16_t frameType = 0;
uint16_t payloadLen = 0;
uint16_t payloadIdx = 0;
uint8_t  payload[LD6002_MAX_PAYLOAD];
uint16_t skipRemaining = 0;

uint32_t framesOk = 0;
uint32_t framesBad = 0;
uint32_t lastFrameMs = 0;

Ld6002Data data = { 0.0f, 0.0f, 0, 0 };

uint8_t checksum(const uint8_t* d, size_t n) {
  uint8_t x = 0;
  for (size_t i = 0; i < n; i++) x ^= d[i];
  return (uint8_t)~x;
}

void publish() { xQueueOverwrite(qLd6002, &data); }

void handleFrame(uint16_t type, const uint8_t* p, uint16_t len) {
  framesOk++;
  lastFrameMs = millis();
#if LD6002_DEBUG
  Serial.printf("[LD6002] type=0x%04X len=%u\n", type, len);
#endif
  if (len < 4) return;
  if (type != TYPE_BREATH && type != TYPE_HEART) return;

  float v;
  memcpy(&v, p, sizeof(v));              // IEEE754 float32, little-endian (ESP32 native)
  if (isnan(v) || v < 0.0f || v > 300.0f) return;

  if (type == TYPE_BREATH) {
    if (v > 0.0f) {                      // 0 = no valid measurement: let it go stale
      data.breathRate = v;
      data.breathTimestampMs = millis();
      publish();
    }
  } else {
    if (v > 0.0f) {
      data.heartRate = v;
      data.heartTimestampMs = millis();
      publish();
    }
  }
}

void startPayloadOrCk() {
  payloadIdx = 0;
  st = (payloadLen > 0) ? Rx::PAYLOAD : Rx::PAYLOAD_CK;
}

void feed(uint8_t b) {
  switch (st) {
    case Rx::SOF:
      if (b == SOF) { head[0] = b; headIdx = 1; st = Rx::HEAD; }
      break;

    case Rx::HEAD:
      head[headIdx++] = b;
      if (headIdx == 8) {
        if (checksum(head, 7) == head[7]) {
          payloadLen = ((uint16_t)head[3] << 8) | head[4];
          frameType  = ((uint16_t)head[5] << 8) | head[6];
          if (payloadLen > LD6002_MAX_PAYLOAD) {
            skipRemaining = payloadLen + 1;       // payload + checksum
            st = Rx::SKIP;
          } else {
            startPayloadOrCk();
          }
        } else {
          framesBad++;
          // resync: look for another SOF inside the rejected header
          uint8_t k = 1;
          while (k < 8 && head[k] != SOF) k++;
          if (k < 8) {
            memmove(head, head + k, 8 - k);
            headIdx = 8 - k;
            st = Rx::HEAD;
          } else {
            st = Rx::SOF;
          }
        }
      }
      break;

    case Rx::PAYLOAD:
      payload[payloadIdx++] = b;
      if (payloadIdx >= payloadLen) st = Rx::PAYLOAD_CK;
      break;

    case Rx::PAYLOAD_CK:
      if (checksum(payload, payloadLen) == b) handleFrame(frameType, payload, payloadLen);
      else framesBad++;
      st = Rx::SOF;
      break;

    case Rx::SKIP:
      if (--skipRemaining == 0) { framesOk++; lastFrameMs = millis(); st = Rx::SOF; }
      break;
  }
}

void ld6002Task(void*) {
  HardwareSerial& radar = Serial2;
  uint32_t baud = LD6002_BAUD_PRIMARY;

  radar.setRxBufferSize(LD6002_RX_BUFFER_BYTES);
  radar.begin(baud, SERIAL_8N1, PIN_LD6002_RX, PIN_LD6002_TX);
  Serial.printf("[LD6002] UART2 started at %lu baud\n", (unsigned long)baud);

  uint32_t lastProbeMs = millis();
  bool locked = false;
  uint32_t okAtLastCheck = 0;

#if SIMULATE_LD6002
  uint32_t lastSim = 0;
#endif

  for (;;) {
    while (radar.available()) feed((uint8_t)radar.read());

    uint32_t now = millis();

    // ---- baud rate lock / probe ----
    if (framesOk >= okAtLastCheck + 3) { locked = true; }
    if (locked && (now - lastFrameMs) > 10000) {
      locked = false; lastProbeMs = now; okAtLastCheck = framesOk;   // need 3 NEW frames to re-lock
    }
    if (!locked && (now - lastProbeMs) > LD6002_BAUD_PROBE_MS) {
      baud = (baud == LD6002_BAUD_PRIMARY) ? LD6002_BAUD_ALTERNATE : LD6002_BAUD_PRIMARY;
      radar.updateBaudRate(baud);
      st = Rx::SOF;
      okAtLastCheck = framesOk;
      lastProbeMs = now;
      Serial.printf("[LD6002] no valid frames (ok=%lu bad=%lu), trying %lu baud\n",
                    (unsigned long)framesOk, (unsigned long)framesBad, (unsigned long)baud);
    }

#if SIMULATE_LD6002
    if (now - lastSim >= 1000) {
      lastSim = now;
      data.breathRate = random(12, 20);
      data.breathTimestampMs = now;
      data.heartRate = random(60, 80);
      data.heartTimestampMs = now;
      publish();
    }
#endif

    vTaskDelay(pdMS_TO_TICKS(LD6002_POLL_MS));
  }
}

} // namespace

void ld6002TaskStart() {
  xTaskCreatePinnedToCore(ld6002Task, "LD6002", STACK_LD6002, nullptr, PRIO_LD6002, nullptr, APP_CORE);
}
