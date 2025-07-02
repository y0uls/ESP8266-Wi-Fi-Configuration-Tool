#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>

enum LedMode { LED_OFF, LED_ON, LED_BLINK_SLOW, LED_BLINK_FAST };
LedMode ledMode = LED_OFF;
unsigned long previousBlinkTime = 0;
bool ledState = false;

const byte DNS_PORT = 53;
DNSServer dnsServer;
ESP8266WebServer server(80);

const char *apSSID          = "APWEMOS";      // SSID AP mode
const char *configPassword  = "admin123";    // config password
String       mdnsName       = "esp8266";      // access "http://esp8266.local"

String ssid = "";
String password = "";

const int ledPin   = 2;
const int resetPin = 0;

bool shouldStartAP         = false;
unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 10'000;
bool wasConnected = false;

void EEPROM_writeString(int addr, const String &data) {
  int len = data.length();
  for (int i = 0; i < len; i++) EEPROM.write(addr + i, data[i]);
  EEPROM.write(addr + len, '\0');
}

String EEPROM_readString(int addr) {
  char data[100];
  int len = 0;
  unsigned char k = EEPROM.read(addr);
  while (k != '\0' && len < 100) {
    data[len++] = k;
    k = EEPROM.read(addr + len);
  }
  data[len] = '\0';
  return String(data);
}

void updateLED() {
  unsigned long now = millis();
  switch (ledMode) {
    case LED_ON:          digitalWrite(ledPin, LOW); break;
    case LED_OFF:         digitalWrite(ledPin, HIGH); break;
    case LED_BLINK_SLOW:  if (now - previousBlinkTime >= 1'000) {
                            previousBlinkTime = now;
                            ledState = !ledState;
                            digitalWrite(ledPin, ledState ? LOW : HIGH);
                          }
                          break;
    case LED_BLINK_FAST:  if (now - previousBlinkTime >= 200) {
                            previousBlinkTime = now;
                            ledState = !ledState;
                            digitalWrite(ledPin, ledState ? LOW : HIGH);
                          }
                          break;
  }
}

String loginPage() {
  return String("<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<style>body{background:#f8f9fa;font-family:Arial;margin:0}"
    ".c{max-width:500px;margin:100px auto;padding:30px;background:#fff;border-radius:10px;"
    "box-shadow:0 0 10px rgba(0,0,0,.1);text-align:center}"
    "h1{font-size:28px;color:#dc3545;margin:0 0 20px}"
    ".i{width:100%;padding:10px;margin-bottom:15px;border:1px solid #ccc;border-radius:5px;font-size:16px;box-sizing:border-box}"
    ".b{width:100%;padding:10px;font-size:16px;font-weight:bold;color:#fff;background:#007bff;border:0;border-radius:5px;cursor:pointer}"
    ".b:hover{background:#0056b3}</style></head><body>"
    "<div class='c'><h1>Welcome to " + String(apSSID) + "</h1>"
    "<form action='/' method='POST'>"
    "<input type='password' name='auth' class='i' placeholder='Password'>"
    "<button type='submit' class='b'>WiFi setup</button>"
    "</form></div></body></html>");
}

String resetPage() {
  return String(F(
    "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<style>body{background:#f8f9fa;font-family:Arial;margin:0}"
    ".c{max-width:500px;margin:100px auto;padding:30px;background:#fff;border-radius:10px;"
    "box-shadow:0 0 10px rgba(0,0,0,.1);text-align:center}"
    "h1{font-size:28px;color:#dc3545;margin:0 0 20px}"
    ".i{width:100%;padding:10px;margin-bottom:15px;border:1px solid #ccc;border-radius:5px;font-size:16px;box-sizing:border-box}"
    ".b{width:100%;padding:10px;font-size:16px;font-weight:bold;color:#fff;background:#007bff;border:0;border-radius:5px;cursor:pointer}"
    ".b:hover{background:#0056b3}"
    ".canc{width:100%;padding:10px;font-size:16px;font-weight:bold;color:#fff;background:#dc3545;border:0;border-radius:5px;cursor:pointer}"
    ".canc:hover{background:#c82333}</style></head><body>"
    "<div class='c'><h1>Reset WiFi Settings</h1>"
    "<form action='/reset' method='POST'>"
    "<input type='password' name='auth' class='i' placeholder='Password'>"
    "<button type='submit' class='b'>Reset Settings</button>"
    "</form>"
    "<br>"
    "<form action='/' method='GET'>"
    "<button type='submit' class='canc'>Cancel</button>"
    "</form>"
    "</div></body></html>"));
}

void handleRoot() {
  bool ok = server.hasArg("auth") && server.arg("auth") == configPassword;

  if (!ok) {
    server.send(200, "text/html", loginPage());
    return;
  }

  String html = R"rawliteral(
  <!DOCTYPE html><html><head>
    <meta name='viewport' content='width=device-width,initial-scale=1'>
    <title>WiFi Configuration</title>
    <style>
      body{background:#f8f9fa;font-family:Arial;margin:0}
      .c{max-width:500px;margin:40px auto;background:#fff;padding:20px;border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,.1)}
      label{font-weight:bold;margin:10px 0 5px;display:block}
      select,input{width:100%;padding:10px;margin-bottom:15px;border:1px solid #ccc;border-radius:5px;box-sizing:border-box}
      .b{width:100%;padding:10px;font-size:16px;font-weight:bold;color:#fff;background:#007bff;border:0;border-radius:5px;cursor:pointer}
      .b:hover{background:#0056b3}
    </style>
    <script>
      function toggleManualSSID(sel){document.getElementById('m').style.display=(sel.value==='__manual__')?'block':'none';}
    </script>
  </head><body>
    <div class='c'>
      <h2 style='text-align:center'>WiFi Configuration</h2>
      <form action='/save' method='POST'>
        <input type='hidden' name='auth' value=')rawliteral" + server.arg("auth") + R"rawliteral('>
        <label>Select a network</label>
        <select name='ssid' onchange='toggleManualSSID(this)'>
  )rawliteral";

  std::vector<String> ssidList;
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    String s = WiFi.SSID(i);
    if (std::find(ssidList.begin(), ssidList.end(), s) == ssidList.end()) {
      ssidList.push_back(s);
      html += "<option>" + s + "</option>";
    }
  }
  html += "<option value='__manual__'>[Enter SSID manually]</option></select>";
  html += F("<div id='m' style='display:none'><label>Custom SSID</label><input name='manual_ssid' type='text'></div>");
  html += F("<label>Password</label><input name='password' type='password'>");
  html += "<button class='b' type='submit'>Save</button></form></div></body></html>";

  server.send(200, "text/html", html);
}

void handleSave() {
  bool ok = server.hasArg("auth") && server.arg("auth") == configPassword;
  if (!ok) { server.send(200, "text/html", loginPage()); return; }

  ssid = server.arg("ssid");
  if (ssid == "__manual__") ssid = server.arg("manual_ssid");
  password = server.arg("password");

  EEPROM.begin(512);
  EEPROM_writeString(0, ssid);
  EEPROM_writeString(100, password);
  EEPROM.commit();
  EEPROM.end();

  String redirectURL = "http://" + mdnsName + ".local/";

  server.send(200, "text/html",
    "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<meta http-equiv='refresh' content='10;url=" + redirectURL + "'>"
    "<script>setTimeout(function(){location.href='" + redirectURL + "';},10000);</script>"
    "<style>body{background:#f8f9fa;font-family:Arial;margin:0}"
    ".c{max-width:500px;margin:100px auto;padding:30px;background:#fff;border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,.1);text-align:center}"
    ".sp{width:3rem;height:3rem;border:.4rem solid #007bff;border-top:.4rem solid transparent;border-radius:50%;animation:spin 1s linear infinite;margin:20px auto}@keyframes spin{0%{transform:rotate(0)}100%{transform:rotate(360deg)}}"
    "</style></head><body><div class='c'><h2>Restarting...</h2><div class='sp'></div>"
    "<p>You will be redirected to <strong>" + redirectURL + "</strong></p></div></body></html>");

  delay(2000);
  ESP.restart();
}

void handleStatus() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>"
                "<title>Status</title>"
                "<style>"
                "body { background:#f8f9fa; font-family:Arial, sans-serif; margin:0; padding:0; }"
                ".container { max-width:500px; margin:40px auto; padding:20px; background:#fff; border-radius:10px; "
                "box-shadow:0 0 10px rgba(0,0,0,0.1); text-align:center; }"
                ".btn { width:100%; padding:12px; font-size:16px; font-weight:bold; color:#fff; "
                "background:#dc3545; border:none; border-radius:5px; cursor:pointer; }"
                ".btn:hover { background:#c82333; }"
                ".info { margin: 15px 0; font-size: 18px; }"
                "</style>"
                "</head><body>"
                "<div class='container'>"
                "<h2>Connected to WiFi</h2>"
                "<p class='info'><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</p>"
                "<p class='info'><strong>SSID:</strong> " + WiFi.SSID() + "</p>"
                "<form action='/reset' method='POST'>"
                "<button type='submit' class='btn'>Reset WiFi Settings</button>"
                "</form>"
                "</div></body></html>";

  server.send(200, "text/html", html);
}

void handleResetPage() {
  bool ok = server.hasArg("auth") && server.arg("auth") == configPassword;
  if (!ok) {
    server.send(200, "text/html", resetPage());
    return;
  }

  EEPROM.begin(512);
  for (int i = 0; i < 512; ++i) EEPROM.write(i, 0);
  EEPROM.commit();
  EEPROM.end();

  String redirectURL = "http://" + mdnsName + ".local/";

  server.send(200, "text/html",
    "<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<meta http-equiv='refresh' content='10;url=" + redirectURL + "'>"
    "<script>setTimeout(function(){location.href='" + redirectURL + "';},10000);</script>"
    "<style>"
      "body{background:#f8f9fa;font-family:Arial;margin:0}"
      ".container{max-width:500px;margin:100px auto;padding:30px;background:#fff;"
        "border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,.1);text-align:center}"
      ".spinner{width:3rem;height:3rem;border:.4rem solid #007bff;"
        "border-top:.4rem solid transparent;border-radius:50%;"
        "animation:spin 1s linear infinite;margin:20px auto}"
      "@keyframes spin{0%{transform:rotate(0)}100%{transform:rotate(360deg)}}"
      ".text-muted{color:#6c757d;font-size:14px}"
    "</style>"
    "</head><body>"
      "<div class='container'>"
        "<h2>Restarting...</h2>"
        "<div class='spinner'></div>"
        "<p class='text-muted'>Redirecting to <strong>" + redirectURL + "</strong> in 10 seconds...</p>"
      "</div>"
    "</body></html>");

  delay(2000);
  ESP.restart();
}

void startAPMode() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/", HTTP_POST, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/reset", HTTP_GET, handleResetPage);
  server.begin();

  ledMode = LED_BLINK_SLOW;
}

void startClientMode() {
  EEPROM.begin(512);
  ssid = EEPROM_readString(0);
  password = EEPROM_readString(100);
  EEPROM.end();

  WiFi.hostname("WEMOS");
  WiFi.begin(ssid.c_str(), password.c_str());

  ledMode = LED_BLINK_FAST;
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10'000) {
    delay(100);
    updateLED();
  }

  if (WiFi.status() != WL_CONNECTED) {
    shouldStartAP = true;
    return;
  }

  ledMode = LED_ON;

  server.on("/", HTTP_GET, handleStatus);
  server.on("/reset", HTTP_POST, handleResetPage);
  server.begin();

  if (MDNS.begin(mdnsName.c_str())) MDNS.addService("http", "tcp", 80);
}

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(resetPin, INPUT_PULLUP);
  Serial.begin(115200);
  delay(1000);

  if (digitalRead(resetPin) == LOW) {
    EEPROM.begin(512);
    for (int i = 0; i < 512; ++i) EEPROM.write(i, 0);
    EEPROM.commit();
    EEPROM.end();
    delay(1000);
  }

  startClientMode();
  if (shouldStartAP) startAPMode();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  MDNS.update();
  updateLED();

  unsigned long now = millis();
  if (now - lastWiFiCheck >= wifiCheckInterval) {
    lastWiFiCheck = now;
    if (WiFi.status() != WL_CONNECTED && wasConnected) {
      delay(1000);
      ESP.restart();
    }
    if (WiFi.status() == WL_CONNECTED) wasConnected = true;
  }
}
