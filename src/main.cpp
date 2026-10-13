#include <Arduino.h>
#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#include <esp_task_wdt.h>
#endif

#include "Config.h"
#include "AppStatus.h"
#include "StateMachine.h"
#include "WebUi.h"

StartLightStateMachine gStateMachine;
WebUi gWebUi(gStateMachine);

namespace {
bool gPirLastRaw = false;
bool gPirStable = false;
unsigned long gPirLastChangeAtMs = 0;

void printPinMap() {
  Serial.println("[PINMAP] Relay mapping:");
  Serial.printf("[PINMAP] Red1: GPIO%u\n", Config::kRelayRed1Pin);
  Serial.printf("[PINMAP] Red2: GPIO%u\n", Config::kRelayRed2Pin);
  Serial.printf("[PINMAP] Red3: GPIO%u\n", Config::kRelayRed3Pin);
  Serial.printf("[PINMAP] Red4: GPIO%u\n", Config::kRelayRed4Pin);
  Serial.printf("[PINMAP] Red5: GPIO%u\n", Config::kRelayRed5Pin);
  Serial.printf("[PINMAP] Buzzer: GPIO%u\n", Config::kRelayBuzzerPin);
  Serial.printf("[PINMAP] PIR: GPIO%u\n", Config::kPirPin);
}

bool pirRisingEdgeDebounced(unsigned long nowMs) {
  const bool raw = digitalRead(Config::kPirPin) ==
                   (Config::kPirActiveHigh ? HIGH : LOW);

  if (raw != gPirLastRaw) {
    gPirLastRaw = raw;
    gPirLastChangeAtMs = nowMs;
  }

  if (nowMs - gPirLastChangeAtMs >= Config::kPirDebounceMs && raw != gPirStable) {
    gPirStable = raw;
    AppStatus::setPirState(gPirLastRaw, gPirStable);
    if (gPirStable) {
      Serial.println("[PIR] Debounced rising edge");
      return true;
    }
  }

  AppStatus::setPirState(gPirLastRaw, gPirStable);
  return false;
}

void setupPins() {
#if defined(ARDUINO_ARCH_ESP8266)
  pinMode(Config::kPirPin, INPUT);
#else
  pinMode(Config::kPirPin, Config::kPirActiveHigh ? INPUT_PULLDOWN : INPUT_PULLUP);
#endif

  for (uint8_t pin : Config::kRedPins) {
    pinMode(pin, OUTPUT);
  }
  pinMode(Config::kBuzzerPin, OUTPUT);

  Config::setAllOff();
}

void runSelfTestIfEnabled() {
  if (!Config::kRunRelaySelfTestOnBoot) {
    return;
  }

  Serial.println("[SELFTEST] Running relay pulse self-test");
  for (uint8_t pin : Config::kRedPins) {
    Config::writeRelay(pin, true);
    delay(Config::kSelfTestPulseMs);
    Config::writeRelay(pin, false);
    delay(40);
  }
  Config::writeBuzzer(true);
  delay(Config::kSelfTestPulseMs);
  Config::writeBuzzer(false);
  Serial.println("[SELFTEST] Complete");
}

void startAccessPoint() {
  WiFi.mode(WIFI_AP);
  const bool started = WiFi.softAP(Config::kApSsid, Config::kApPassword);
  if (!started) {
    Serial.println("[FAULT] Failed to start AP");
    return;
  }

  IPAddress ip = WiFi.softAPIP();
  Serial.printf("[NET] AP started. SSID=%s IP=%s\n", Config::kApSsid,
                ip.toString().c_str());
}

void setupWatchdog() {
#if defined(ARDUINO_ARCH_ESP8266)
  ESP.wdtEnable(8000);
#else
  if (esp_task_wdt_init(Config::kWatchdogTimeoutSeconds, true) != ESP_OK) {
    Serial.println("[WARN] Watchdog init failed");
    return;
  }

  if (esp_task_wdt_add(NULL) != ESP_OK) {
    Serial.println("[WARN] Watchdog add current task failed");
  }
#endif
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n[BOOT] F1 Start Light booting");
  printPinMap();

  setupPins();
  runSelfTestIfEnabled();

  setupWatchdog();
  startAccessPoint();

  gStateMachine.begin();
  gWebUi.begin();

  Serial.println("[BOOT] Ready");
}

void loop() {
  const unsigned long nowMs = millis();
  const bool pirEdge = pirRisingEdgeDebounced(nowMs);

  gStateMachine.loop(nowMs, pirEdge);
  gWebUi.loop();

#if defined(ARDUINO_ARCH_ESP8266)
  ESP.wdtFeed();
#else
  esp_task_wdt_reset();
#endif
}
