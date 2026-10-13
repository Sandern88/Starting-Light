#include "StateMachine.h"

#include "Config.h"

namespace {
const char* kStateNames[] = {
  "IDLE", "ARMED_WAIT_PIR", "COUNTDOWN", "COMPLETE", "LOCKOUT",
  "HARDWARE_TEST", "FAULT",
};
}

void StartLightStateMachine::begin() {
  allOutputsOff();
  enterState(armed_ ? SequenceState::ARMED_WAIT_PIR : SequenceState::IDLE, "boot");
}

void StartLightStateMachine::loop(unsigned long nowMs, bool pirRisingEdge) {
  if (pirRisingEdge && state_ == SequenceState::ARMED_WAIT_PIR) {
    lastTriggerAtMs_ = nowMs;
    startCountdown("pir");
    return;
  }

  if (buzzerOn_ && buzzerOffAtMs_ != 0 && nowMs >= buzzerOffAtMs_) {
    buzzerOn_ = false;
    buzzerOffAtMs_ = 0;
    applyOutputs();
  }

  switch (state_) {
    case SequenceState::IDLE:
      if (armed_) {
        enterState(SequenceState::ARMED_WAIT_PIR, "armed");
      }
      break;

    case SequenceState::ARMED_WAIT_PIR:
      if (!armed_) {
        enterState(SequenceState::IDLE, "disarmed");
      }
      break;

    case SequenceState::COUNTDOWN:
      if (litRedCount_ < 5 && nowMs - countdownLastStepAtMs_ >= timings_.redStepMs) {
        ++litRedCount_;
        countdownLastStepAtMs_ = nowMs;
        buzzerOn_ = true;
        buzzerOffAtMs_ = nowMs + timings_.buzzerBeepMs;
        applyOutputs();
        Serial.printf("[SEQ] Red step %u/5\n", litRedCount_);
      }

      if (litRedCount_ == 5 && allOffAtMs_ == 0) {
        allOffAtMs_ = nowMs + timings_.allOffDelayMs;
      }

      if (allOffAtMs_ != 0 && nowMs >= allOffAtMs_) {
        litRedCount_ = 0;
        buzzerOn_ = true;
        buzzerOffAtMs_ = nowMs + timings_.buzzerFinalHoldMs;
        applyOutputs();
        enterState(SequenceState::COMPLETE, "countdown finished");
      }
      break;

    case SequenceState::COMPLETE:
      if (nowMs - stateEnteredAtMs_ >= timings_.completeStateMs) {
        enterState(SequenceState::LOCKOUT, "complete delay done");
      }
      break;

    case SequenceState::LOCKOUT:
      if (nowMs - stateEnteredAtMs_ >= timings_.lockoutMs) {
        enterState(armed_ ? SequenceState::ARMED_WAIT_PIR : SequenceState::IDLE,
                   "lockout done");
      }
      break;

    case SequenceState::HARDWARE_TEST:
      if (!hardwareTestActive_) {
        enterState(armed_ ? SequenceState::ARMED_WAIT_PIR : SequenceState::IDLE,
                   "hardware test stopped");
        break;
      }

      if (nowMs - hardwareLastStepAtMs_ >= hardwareStepMs_) {
        ++hardwareTestChannel_;
        hardwareLastStepAtMs_ = nowMs;

        if (hardwareTestChannel_ > 5) {
          stopHardwareTest();
          enterState(armed_ ? SequenceState::ARMED_WAIT_PIR : SequenceState::IDLE,
                     "hardware test complete");
          break;
        }

        applyHardwareTestOutputs();
        Serial.printf("[HWTEST] Channel %u active\n", hardwareTestChannel_);
      }
      break;

    case SequenceState::FAULT:
      allOutputsOff();
      break;
  }
}

void StartLightStateMachine::arm() {
  armed_ = true;
  if (state_ == SequenceState::IDLE) {
    enterState(SequenceState::ARMED_WAIT_PIR, "arm cmd");
  }
}

void StartLightStateMachine::disarm() {
  armed_ = false;
  if (state_ == SequenceState::ARMED_WAIT_PIR) {
    enterState(SequenceState::IDLE, "disarm cmd");
  }
}

void StartLightStateMachine::manualStart() {
  if (state_ == SequenceState::ARMED_WAIT_PIR) {
    lastTriggerAtMs_ = millis();
    startCountdown("manual start");
    return;
  }

  Serial.printf("[WARN] Manual start ignored in state %s\n", stateName());
}

void StartLightStateMachine::abort() {
  litRedCount_ = 0;
  buzzerOn_ = false;
  buzzerOffAtMs_ = 0;
  allOffAtMs_ = 0;
  hardwareTestActive_ = false;
  applyOutputs();
  enterState(armed_ ? SequenceState::ARMED_WAIT_PIR : SequenceState::IDLE, "abort cmd");
}

void StartLightStateMachine::resetFault() {
  if (state_ == SequenceState::FAULT) {
    litRedCount_ = 0;
    buzzerOn_ = false;
    buzzerOffAtMs_ = 0;
    applyOutputs();
    enterState(armed_ ? SequenceState::ARMED_WAIT_PIR : SequenceState::IDLE,
               "fault reset");
  }
}

SequenceState StartLightStateMachine::state() const { return state_; }

const char* StartLightStateMachine::stateName() const {
  return kStateNames[static_cast<size_t>(state_)];
}

bool StartLightStateMachine::isArmed() const { return armed_; }

uint8_t StartLightStateMachine::litRedCount() const { return litRedCount_; }

bool StartLightStateMachine::buzzerOn() const { return buzzerOn_; }

unsigned long StartLightStateMachine::stateEnteredAtMs() const {
  return stateEnteredAtMs_;
}

unsigned long StartLightStateMachine::lastTriggerAtMs() const {
  return lastTriggerAtMs_;
}

bool StartLightStateMachine::updateTimings(const SequenceTimings& next) {
  if (!validateTimings(next)) {
    return false;
  }

  timings_ = next;
  Serial.printf(
      "[CFG] Timings updated redStep=%lu allOff=%lu complete=%lu lockout=%lu "
      "buzzerBeep=%lu buzzerHold=%lu\n",
      timings_.redStepMs, timings_.allOffDelayMs, timings_.completeStateMs,
      timings_.lockoutMs, timings_.buzzerBeepMs, timings_.buzzerFinalHoldMs);
  return true;
}

SequenceTimings StartLightStateMachine::timings() const {
  return timings_;
}

bool StartLightStateMachine::startHardwareTest(unsigned long stepMs) {
  if (state_ == SequenceState::COUNTDOWN || state_ == SequenceState::COMPLETE ||
      state_ == SequenceState::LOCKOUT) {
    return false;
  }

  if (stepMs < 150 || stepMs > 5000) {
    return false;
  }

  hardwareTestActive_ = true;
  hardwareStepMs_ = stepMs;
  hardwareTestChannel_ = 0;
  hardwareLastStepAtMs_ = millis();

  applyHardwareTestOutputs();
  enterState(SequenceState::HARDWARE_TEST, "hardware test start");
  Serial.printf("[HWTEST] Started with stepMs=%lu\n", hardwareStepMs_);
  return true;
}

void StartLightStateMachine::stopHardwareTest() {
  hardwareTestActive_ = false;
  hardwareTestChannel_ = 0;
  hardwareStepMs_ = 0;
  allOutputsOff();
  Serial.println("[HWTEST] Stopped");
}

bool StartLightStateMachine::isHardwareTestActive() const {
  return state_ == SequenceState::HARDWARE_TEST && hardwareTestActive_;
}

uint8_t StartLightStateMachine::hardwareTestChannel() const {
  return hardwareTestChannel_;
}

void StartLightStateMachine::enterState(SequenceState nextState, const char* reason) {
  if (state_ == nextState) {
    return;
  }

  Serial.printf("[STATE] %s -> %s (%s)\n", stateName(),
                kStateNames[static_cast<size_t>(nextState)], reason);

  state_ = nextState;
  stateEnteredAtMs_ = millis();

  if (state_ != SequenceState::COUNTDOWN) {
    allOffAtMs_ = 0;
  }

  if (state_ == SequenceState::IDLE || state_ == SequenceState::ARMED_WAIT_PIR ||
      state_ == SequenceState::COMPLETE || state_ == SequenceState::LOCKOUT ||
      state_ == SequenceState::HARDWARE_TEST ||
      state_ == SequenceState::FAULT) {
    litRedCount_ = 0;

    // COMPLETE keeps the buzzer running for its final hold beep set just before this call.
    if (state_ != SequenceState::COMPLETE) {
      buzzerOn_ = false;
      buzzerOffAtMs_ = 0;
    }

    if (state_ == SequenceState::HARDWARE_TEST && hardwareTestActive_) {
      applyHardwareTestOutputs();
    } else {
      applyOutputs();
    }
  }
}

void StartLightStateMachine::startCountdown(const char* reason) {
  if (!armed_) {
    enterFault("countdown while disarmed");
    return;
  }

  if (hardwareTestActive_) {
    stopHardwareTest();
  }

  litRedCount_ = 0;
  buzzerOn_ = false;
  buzzerOffAtMs_ = 0;
  countdownLastStepAtMs_ = millis();
  allOffAtMs_ = 0;
  applyOutputs();
  enterState(SequenceState::COUNTDOWN, reason);
}

void StartLightStateMachine::enterFault(const char* reason) {
  Serial.printf("[FAULT] %s\n", reason);
  enterState(SequenceState::FAULT, reason);
}

void StartLightStateMachine::applyOutputs() {
  for (size_t i = 0; i < 5; ++i) {
    Config::writeRelay(Config::kRedPins[i], i < litRedCount_);
  }
  Config::writeBuzzer(buzzerOn_);
}

void StartLightStateMachine::allOutputsOff() {
  Config::setAllOff();
}

bool StartLightStateMachine::validateTimings(const SequenceTimings& candidate) const {
  if (candidate.redStepMs < 250 || candidate.redStepMs > 5000) {
    return false;
  }
  if (candidate.allOffDelayMs > 3000) {
    return false;
  }
  if (candidate.completeStateMs > 3000) {
    return false;
  }
  if (candidate.lockoutMs < 500 || candidate.lockoutMs > 30000) {
    return false;
  }
  if (candidate.buzzerBeepMs < 20 || candidate.buzzerBeepMs > 2000) {
    return false;
  }
  if (candidate.buzzerFinalHoldMs < 20 || candidate.buzzerFinalHoldMs > 5000) {
    return false;
  }

  return true;
}

void StartLightStateMachine::applyHardwareTestOutputs() {
  allOutputsOff();

  if (hardwareTestChannel_ < 5) {
    Config::writeRelay(Config::kRedPins[hardwareTestChannel_], true);
    return;
  }

  if (hardwareTestChannel_ == 5) {
    Config::writeBuzzer(true);
  }
}
