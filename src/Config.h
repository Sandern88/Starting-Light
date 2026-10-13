#pragma once

#include <Arduino.h>

namespace Config {
constexpr char kDeviceName[] = "F1-Start-Light";
constexpr char kApSsid[] = "F1StartLight";
constexpr char kApPassword[] = "startlight123";

constexpr bool kRelayActiveLow = true;
constexpr bool kBuzzerActiveLow = false;  // buzzer wired directly to GPIO, not through the relay board
constexpr uint8_t kPirPin = 7;

// Relay input mapping (ESP32-S2 GPIO -> relay board INx)
constexpr uint8_t kRelayRed1Pin = 1;
constexpr uint8_t kRelayRed2Pin = 2;
constexpr uint8_t kRelayRed3Pin = 3;
constexpr uint8_t kRelayRed4Pin = 4;
constexpr uint8_t kRelayRed5Pin = 5;
constexpr uint8_t kRelayBuzzerPin = 6;

constexpr uint8_t kRedPins[5] = {
  kRelayRed1Pin,
  kRelayRed2Pin,
  kRelayRed3Pin,
  kRelayRed4Pin,
  kRelayRed5Pin,
};
constexpr uint8_t kBuzzerPin = kRelayBuzzerPin;

constexpr bool kPirActiveHigh = true;
constexpr unsigned long kPirDebounceMs = 150;

constexpr unsigned long kBuzzerBeepMs = 800;
constexpr unsigned long kBuzzerFinalHoldMs = 2500;

constexpr unsigned long kDefaultRedStepMs = 1900;
constexpr unsigned long kDefaultAllOffDelayMs = 2000;
constexpr unsigned long kDefaultCompleteStateMs = 2500;
constexpr unsigned long kDefaultLockoutMs = 5000;

constexpr unsigned long kWatchdogTimeoutSeconds = 8;
constexpr bool kRunRelaySelfTestOnBoot = false;
constexpr unsigned long kSelfTestPulseMs = 120;

inline void writeRelay(uint8_t pin, bool on) {
  digitalWrite(pin, on ? (kRelayActiveLow ? LOW : HIGH)
                       : (kRelayActiveLow ? HIGH : LOW));
}

inline void writeBuzzer(bool on) {
  digitalWrite(kBuzzerPin, on ? (kBuzzerActiveLow ? LOW : HIGH)
                             : (kBuzzerActiveLow ? HIGH : LOW));
}

inline void setAllOff() {
  for (uint8_t pin : kRedPins) {
    writeRelay(pin, false);
  }
  writeBuzzer(false);
}
}  // namespace Config
