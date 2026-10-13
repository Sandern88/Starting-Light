#include "WebUi.h"

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#include "Config.h"
#include "AppStatus.h"

WebUi::WebUi(StartLightStateMachine& sm) : sm_(sm) {}

void WebUi::begin() {
  server_.on("/", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  server_.on("/api/arm", HTTP_POST, [this]() { handleArm(); });
  server_.on("/api/disarm", HTTP_POST, [this]() { handleDisarm(); });
  server_.on("/api/start", HTTP_POST, [this]() { handleStart(); });
  server_.on("/api/abort", HTTP_POST, [this]() { handleAbort(); });
  server_.on("/api/reset-fault", HTTP_POST, [this]() { handleResetFault(); });
  server_.on("/api/timings", HTTP_POST, [this]() { handleSetTimings(); });
  server_.on("/api/hwtest/run", HTTP_POST, [this]() { handleHwTestRun(); });
  server_.on("/api/hwtest/stop", HTTP_POST, [this]() { handleHwTestStop(); });
  server_.begin();
}

void WebUi::loop() { server_.handleClient(); }

String WebUi::renderHtml() const {
  String html;
  html.reserve(9000);
  html += "<!doctype html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>F1 Start Light</title>";
  html += "<style>body{font-family:Arial,sans-serif;background:#101820;color:#f3f3f3;padding:20px;}";
  html += ".card{max-width:920px;margin:auto;background:#1b2638;padding:18px;border-radius:14px;box-shadow:0 14px 30px rgba(0,0,0,.25);}"
          ".row{display:flex;gap:10px;flex-wrap:wrap;margin-top:12px;}";
  html += ".section{margin-top:18px;padding-top:12px;border-top:1px solid #2f4058;}";
  html += ".hero{display:grid;grid-template-columns:1.35fr .65fr;gap:12px;align-items:stretch;}";
  html += ".hero-box{background:linear-gradient(180deg,#0b1320,#132033);border:1px solid #2d415c;border-radius:16px;padding:16px;min-height:120px;}";
  html += ".hero-title{font-size:14px;letter-spacing:.12em;text-transform:uppercase;color:#94a3b8;margin:0 0 8px 0;}";
  html += ".hero-state{font-size:28px;font-weight:700;line-height:1.1;margin:0;}";
  html += ".hero-sub{margin-top:8px;color:#cbd5e1;font-size:13px;line-height:1.45;}";
  html += ".hero-ring{display:flex;align-items:center;justify-content:center;background:radial-gradient(circle at 50% 35%, rgba(255,255,255,.1), rgba(255,255,255,0) 60%), linear-gradient(180deg,#0b1320,#132033);border:1px solid #2d415c;border-radius:16px;min-height:120px;}";
  html += ".big-dot{width:64px;height:64px;border-radius:50%;background:#64748b;box-shadow:0 0 0 6px rgba(255,255,255,.05) inset, 0 0 28px rgba(0,0,0,.25);}"
    ".lamp-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:12px;margin-top:12px;}";
  html += ".lamp{background:#0b1320;border:1px solid #2d415c;border-radius:14px;padding:14px;text-align:center;min-height:118px;display:flex;flex-direction:column;justify-content:center;}";
  html += ".dot{width:22px;height:22px;border-radius:50%;display:inline-block;margin:0 auto 10px auto;background:#64748b;box-shadow:0 0 0 2px rgba(255,255,255,.08) inset;}";
  html += ".on{background:#22c55e;box-shadow:0 0 18px rgba(34,197,94,.70);} .off{background:#64748b;} .warn{background:#f59e0b;box-shadow:0 0 18px rgba(245,158,11,.52);} .danger{background:#ef4444;box-shadow:0 0 18px rgba(239,68,68,.60);} .muted{opacity:.45;}";
  html += "button{padding:10px 14px;border:none;border-radius:8px;background:#e63946;color:#fff;cursor:pointer;}";
  html += "label{display:block;margin-top:8px;font-size:13px;color:#d6e0ef;}";
  html += "input{width:120px;padding:8px;border-radius:6px;border:1px solid #475569;background:#0b1320;color:#fff;}";
  html += "button.alt{background:#457b9d;}button.warn{background:#f77f00;}";
  html += "pre{background:#0b1320;padding:10px;border-radius:8px;overflow:auto;}";
  html += "</style></head><body><div class='card'>";
  html += "<h2>F1 Start Light Controller</h2>";
  html += "<p>ESP32-S2 AP: <b>" + String(Config::kApSsid) + "</b></p>";
  html += "<div class='section hero'><div class='hero-box'><div class='hero-title'>Current state</div><div class='hero-state' id='heroState'>Loading...</div><div class='hero-sub' id='heroSub'>Waiting for data</div></div><div class='hero-ring'><div class='big-dot' id='heroDot'></div></div></div>";
  html += "<div class='section'><div class='lamp-grid' id='stateLamps'></div></div>";
  html += "<div class='section'><h3>Inputs</h3><div class='lamp-grid' id='inputLamps'></div></div>";
  html += "<div class='section'><h3>Outputs</h3><div class='lamp-grid' id='outputLamps'></div></div>";
  html += "<div class='section'><h3>Raw status</h3><pre id='status'>loading...</pre></div>";
  html += "<div class='row'>";
  html += "<button onclick=\"post('/api/arm')\">Arm</button>";
  html += "<button class='alt' onclick=\"post('/api/disarm')\">Disarm</button>";
  html += "<button onclick=\"post('/api/start')\">Start Test</button>";
  html += "<button class='warn' onclick=\"post('/api/abort')\">Abort</button>";
  html += "<button class='warn' onclick=\"post('/api/reset-fault')\">Reset Fault</button>";
  html += "</div>";
  html += "<h3>Live timings</h3>";
  html += "<div class='row'>";
  html += "<label>Red step ms<br><input id='redStepMs' type='number' min='250' max='5000' value='1000'></label>";
  html += "<label>All off delay ms<br><input id='allOffDelayMs' type='number' min='0' max='3000' value='300'></label>";
  html += "<label>Complete ms<br><input id='completeStateMs' type='number' min='0' max='3000' value='700'></label>";
  html += "<label>Lockout ms<br><input id='lockoutMs' type='number' min='500' max='30000' value='5000'></label>";
  html += "<label>Buzzer beep ms<br><input id='buzzerBeepMs' type='number' min='20' max='2000' value='100'></label>";
  html += "<label>Buzzer hold ms<br><input id='buzzerFinalHoldMs' type='number' min='20' max='5000' value='300'></label>";
  html += "</div>";
  html += "<div class='row'><button class='alt' onclick='saveTimings()'>Save Timings</button></div>";
  html += "<h3>Hardware test</h3>";
  html += "<p>Runs each channel one-by-one: Red1..Red5, then Buzzer.</p>";
  html += "<div class='row'>";
  html += "<label>Step ms<br><input id='hwStepMs' type='number' min='150' max='5000' value='700'></label>";
  html += "</div>";
  html += "<div class='row'>";
  html += "<button onclick='runHwTest()'>Run Hardware Test</button>";
  html += "<button class='warn' onclick=\"post('/api/hwtest/stop')\">Stop Hardware Test</button>";
  html += "</div>";
  html += "</div><script>";
  html += "const stateMeta=[{k:'IDLE',l:'Idle',c:'off'},{k:'ARMED_WAIT_PIR',l:'Armed',c:'on'},{k:'COUNTDOWN',l:'Countdown',c:'warn'},{k:'COMPLETE',l:'Complete',c:'on'},{k:'LOCKOUT',l:'Lockout',c:'warn'},{k:'HARDWARE_TEST',l:'HW test',c:'warn'},{k:'FAULT',l:'Fault',c:'danger'}];";
  html += "function lamp(label,on,cls='off',sub=''){return `<div class='lamp'><div class='dot ${on?cls:'off'}'></div><div><b>${label}</b></div><div style='font-size:12px;opacity:.85'>${sub}</div></div>`;}";
  html += "function renderLamps(d){";
  html += "const meta=stateMeta.find(x=>x.k===d.state)||stateMeta[0];";
  html += "document.getElementById('heroState').textContent=meta.l;";
  html += "document.getElementById('heroSub').textContent=`PIR ${d.pirStableHigh?'HIGH':'LOW'} | `+`Armed ${d.armed?'ON':'OFF'} | `+`Red ${d.litRedCount||0}/5`+((d.hardwareTestActive)?` | HW ch ${d.hardwareTestChannel}`:'');";
  html += "document.getElementById('heroDot').className='big-dot '+meta.c;";
  html += "const stateBox=stateMeta.map(x=>lamp(x.l,d.state===x.k,x.c,d.state===x.k?'active':'' )).join('');";
  html += "document.getElementById('stateLamps').innerHTML=stateBox;";
  html += "const pirStable=d.pirStableHigh||false; const pirRaw=d.pirRawHigh||false;";
  html += "document.getElementById('inputLamps').innerHTML=[";
  html += "lamp('PIR input',pirRaw,'on',pirRaw?'wired HIGH':'wired LOW'),";
  html += "lamp('PIR raw',pirRaw,'on',pirRaw?'HIGH':'LOW'),";
  html += "lamp('PIR stable',pirStable,'on',pirStable?'HIGH':'LOW'),";
  html += "lamp('Armed',d.armed,'on',d.armed?'armed':'idle'),";
  html += "lamp('HW test',d.hardwareTestActive,'warn',d.hardwareTestActive?('ch '+d.hardwareTestChannel):'off')";
  html += "].join('');";
  html += "const redCount=d.litRedCount||0;";
  html += "const outputs=[];";
  html += "for(let i=1;i<=5;i++){outputs.push(lamp('Red '+i,i<=redCount,d.state==='COUNTDOWN'?'danger':'on',i<=redCount?'ON':'off'));}";
  html += "outputs.push(lamp('Buzzer',d.buzzerOn,'warn',d.buzzerOn?'on':'off'));";
  html += "document.getElementById('outputLamps').innerHTML=outputs.join('');";
  html += "}";
  html += "async function refresh(){const r=await fetch('/api/status');const d=await r.json();";
  html += "document.getElementById('status').textContent=JSON.stringify(d,null,2);renderLamps(d);";
  html += "if(d.timings&&!timingsBeingEdited){document.getElementById('redStepMs').value=d.timings.redStepMs;";
  html += "document.getElementById('allOffDelayMs').value=d.timings.allOffDelayMs;";
  html += "document.getElementById('completeStateMs').value=d.timings.completeStateMs;";
  html += "document.getElementById('lockoutMs').value=d.timings.lockoutMs;";
  html += "document.getElementById('buzzerBeepMs').value=d.timings.buzzerBeepMs;";
  html += "document.getElementById('buzzerFinalHoldMs').value=d.timings.buzzerFinalHoldMs;}}";
  html += "async function post(url){await fetch(url,{method:'POST'});refresh();}";
  html += "async function saveTimings(){";
  html += "const p=new URLSearchParams();";
  html += "p.set('redStepMs',document.getElementById('redStepMs').value);";
  html += "p.set('allOffDelayMs',document.getElementById('allOffDelayMs').value);";
  html += "p.set('completeStateMs',document.getElementById('completeStateMs').value);";
  html += "p.set('lockoutMs',document.getElementById('lockoutMs').value);";
  html += "p.set('buzzerBeepMs',document.getElementById('buzzerBeepMs').value);";
  html += "p.set('buzzerFinalHoldMs',document.getElementById('buzzerFinalHoldMs').value);";
  html += "await fetch('/api/timings?'+p.toString(),{method:'POST'});refresh();}";
  html += "async function runHwTest(){";
  html += "const ms=document.getElementById('hwStepMs').value;";
  html += "await fetch('/api/hwtest/run?stepMs='+encodeURIComponent(ms),{method:'POST'});refresh();}";
  html += "setInterval(refresh,1000);refresh();";
  html += "let timingsBeingEdited=false;";
  html += "for(const id of ['redStepMs','allOffDelayMs','completeStateMs','lockoutMs','buzzerBeepMs','buzzerFinalHoldMs']){";
  html += "const el=document.getElementById(id);";
  html += "el.addEventListener('focus',()=>{timingsBeingEdited=true;});";
  html += "el.addEventListener('blur',()=>{timingsBeingEdited=false;});}";
  html += "</script></body></html>";
  return html;
}

String WebUi::renderStatusJson() const {
  const SequenceTimings t = sm_.timings();
  const bool pirRawHigh = AppStatus::pirRawHigh();
  const bool pirStableHigh = AppStatus::pirStableHigh();
  String body = "{";
  body += "\"device\":\"" + String(Config::kDeviceName) + "\",";
  body += "\"state\":\"" + String(sm_.stateName()) + "\",";
  body += "\"armed\":" + String(sm_.isArmed() ? "true" : "false") + ",";
  body += "\"pirInputHigh\":" + String(pirRawHigh ? "true" : "false") + ",";
  body += "\"pirRawHigh\":" + String(pirRawHigh ? "true" : "false") + ",";
  body += "\"pirStableHigh\":" + String(pirStableHigh ? "true" : "false") + ",";
  body += "\"litRedCount\":" + String(sm_.litRedCount()) + ",";
  body += "\"buzzerOn\":" + String(sm_.buzzerOn() ? "true" : "false") + ",";
  body += "\"hardwareTestActive\":" +
          String(sm_.isHardwareTestActive() ? "true" : "false") + ",";
  body += "\"hardwareTestChannel\":" + String(sm_.hardwareTestChannel()) + ",";
  body += "\"uptimeMs\":" + String(millis()) + ",";
  body += "\"lastTriggerAtMs\":" + String(sm_.lastTriggerAtMs()) + ",";
  body += "\"timings\":{";
  body += "\"redStepMs\":" + String(t.redStepMs) + ",";
  body += "\"allOffDelayMs\":" + String(t.allOffDelayMs) + ",";
  body += "\"completeStateMs\":" + String(t.completeStateMs) + ",";
  body += "\"lockoutMs\":" + String(t.lockoutMs) + ",";
  body += "\"buzzerBeepMs\":" + String(t.buzzerBeepMs) + ",";
  body += "\"buzzerFinalHoldMs\":" + String(t.buzzerFinalHoldMs);
  body += "}";
  body += "}";
  return body;
}

void WebUi::handleRoot() {
  server_.send(200, "text/html", renderHtml());
}

void WebUi::handleStatus() {
  server_.send(200, "application/json", renderStatusJson());
}

void WebUi::handleArm() {
  sm_.arm();
  server_.send(200, "text/plain", "armed");
}

void WebUi::handleDisarm() {
  sm_.disarm();
  server_.send(200, "text/plain", "disarmed");
}

void WebUi::handleStart() {
  sm_.manualStart();
  server_.send(200, "text/plain", "start requested");
}

void WebUi::handleAbort() {
  sm_.abort();
  server_.send(200, "text/plain", "aborted");
}

void WebUi::handleResetFault() {
  sm_.resetFault();
  server_.send(200, "text/plain", "fault reset requested");
}

void WebUi::handleSetTimings() {
  SequenceTimings next = sm_.timings();

  if (!server_.hasArg("redStepMs") || !server_.hasArg("allOffDelayMs") ||
      !server_.hasArg("completeStateMs") || !server_.hasArg("lockoutMs") ||
      !server_.hasArg("buzzerBeepMs") || !server_.hasArg("buzzerFinalHoldMs")) {
    server_.send(400, "text/plain", "missing timing args");
    return;
  }

  next.redStepMs = static_cast<unsigned long>(server_.arg("redStepMs").toInt());
  next.allOffDelayMs =
      static_cast<unsigned long>(server_.arg("allOffDelayMs").toInt());
  next.completeStateMs =
      static_cast<unsigned long>(server_.arg("completeStateMs").toInt());
  next.lockoutMs = static_cast<unsigned long>(server_.arg("lockoutMs").toInt());
  next.buzzerBeepMs =
      static_cast<unsigned long>(server_.arg("buzzerBeepMs").toInt());
  next.buzzerFinalHoldMs =
      static_cast<unsigned long>(server_.arg("buzzerFinalHoldMs").toInt());

  if (!sm_.updateTimings(next)) {
    server_.send(400, "text/plain", "invalid timing values");
    return;
  }

  server_.send(200, "text/plain", "timings updated");
}

void WebUi::handleHwTestRun() {
  if (!server_.hasArg("stepMs")) {
    server_.send(400, "text/plain", "missing stepMs");
    return;
  }

  const unsigned long stepMs =
      static_cast<unsigned long>(server_.arg("stepMs").toInt());
  if (!sm_.startHardwareTest(stepMs)) {
    server_.send(409, "text/plain", "hardware test rejected in current state");
    return;
  }

  server_.send(200, "text/plain", "hardware test started");
}

void WebUi::handleHwTestStop() {
  sm_.stopHardwareTest();
  server_.send(200, "text/plain", "hardware test stopped");
}
