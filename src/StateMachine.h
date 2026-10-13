#pragma once

#include <Arduino.h>

#include "Config.h"

enum class SequenceState {
  IDLE,
  ARMED_WAIT_PIR,
  COUNTDOWN,
  COMPLETE,
  LOCKOUT,
  HARDWARE_TEST,
  FAULT,
};

struct SequenceTimings {
  unsigned long redStepMs;
  unsigned long allOffDelayMs;
  unsigned long completeStateMs;
  unsigned long lockoutMs;
  unsigned long buzzerBeepMs;
  unsigned long buzzerFinalHoldMs;
};

class StartLightStateMachine {
 public:
  void begin();
  void loop(unsigned long nowMs, bool pirRisingEdge);

  void arm();
  void disarm();
  void manualStart();
  void abort();
  void resetFault();

  bool updateTimings(const SequenceTimings& next);
  SequenceTimings timings() const;

  bool startHardwareTest(unsigned long stepMs);
  void stopHardwareTest();
  bool isHardwareTestActive() const;
  uint8_t hardwareTestChannel() const;

  SequenceState state() const;
  const char* stateName() const;
  bool isArmed() const;
  uint8_t litRedCount() const;
  bool buzzerOn() const;
  unsigned long stateEnteredAtMs() const;
  unsigned long lastTriggerAtMs() const;

 private:
  void enterState(SequenceState nextState, const char* reason);
  void startCountdown(const char* reason);
  void enterFault(const char* reason);
  void applyOutputs();
  void allOutputsOff();
  bool validateTimings(const SequenceTimings& candidate) const;
  void applyHardwareTestOutputs();

  SequenceState state_ = SequenceState::IDLE;
  bool armed_ = true;
  uint8_t litRedCount_ = 0;
  bool buzzerOn_ = false;
  unsigned long stateEnteredAtMs_ = 0;
  unsigned long countdownLastStepAtMs_ = 0;
  unsigned long allOffAtMs_ = 0;
  unsigned long buzzerOffAtMs_ = 0;
  unsigned long lastTriggerAtMs_ = 0;

  SequenceTimings timings_ = {
      Config::kDefaultRedStepMs,
      Config::kDefaultAllOffDelayMs,
      Config::kDefaultCompleteStateMs,
      Config::kDefaultLockoutMs,
      Config::kBuzzerBeepMs,
      Config::kBuzzerFinalHoldMs,
  };

  bool hardwareTestActive_ = false;
  uint8_t hardwareTestChannel_ = 0;
  unsigned long hardwareStepMs_ = 0;
  unsigned long hardwareLastStepAtMs_ = 0;
};
