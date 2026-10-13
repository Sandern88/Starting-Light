#pragma once

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WebServer.h>
using WebServerType = ESP8266WebServer;
#else
#include <WebServer.h>
using WebServerType = WebServer;
#endif

#include "StateMachine.h"

class WebUi {
 public:
  explicit WebUi(StartLightStateMachine& sm);

  void begin();
  void loop();

 private:
  String renderHtml() const;
  String renderStatusJson() const;

  void handleRoot();
  void handleStatus();
  void handleArm();
  void handleDisarm();
  void handleStart();
  void handleAbort();
  void handleResetFault();
  void handleSetTimings();
  void handleHwTestRun();
  void handleHwTestStop();

  StartLightStateMachine& sm_;
  WebServerType server_{80};
};
