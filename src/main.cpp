#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"

#define MAX_DEVICES 20

//  Upstream
const char* upstream_ssid = "jarvis";
const char* upstream_pass = "liks6228";

//  AP
const char* ssid = "Hawk_SUB0";
const char* password = "12345678";

// Friendly
String friendlyMACs[] = {
  "F6:8E:54:52:C8:E7",
  "22:ED:FC:E2:D0:A5",
  "BE:91:E4:2F:08:AA"
};
int friendlyCount = 3;

// Tracking
String knownDevices[MAX_DEVICES];
String deviceNames[MAX_DEVICES];
unsigned long connectTime[MAX_DEVICES];
int reconnectCount[MAX_DEVICES];
int deviceCount = 0;

String prevDevices[MAX_DEVICES];
int prevCount = 0;

//  Naming
String generateName(int index) {
  char letter = 'A' + index;
  return String(letter) + String(index + 1);
}

//  Friendly check
bool isFriendly(String mac) {
  for (int i = 0; i < friendlyCount; i++) {
    if (mac == friendlyMACs[i]) return true;
  }
  return false;
}

//  Upstream connect
void connectToUpstream() {
  Serial.println("Connecting...");
  WiFi.begin(upstream_ssid, upstream_pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n Connected to Network Source ");
  Serial.println(WiFi.localIP());
}

//  MAIN
void printClients() {
  wifi_sta_list_t stationList;
  tcpip_adapter_sta_list_t adapterList;

  esp_wifi_ap_get_sta_list(&stationList);
  tcpip_adapter_get_sta_list(&stationList, &adapterList);

  String currentDevices[MAX_DEVICES];
  int currentCount = 0;

  // Collect
  for (int i = 0; i < adapterList.num; i++) {
    tcpip_adapter_sta_info_t station = adapterList.sta[i];

    char mac[18];
    sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X",
      station.mac[0], station.mac[1], station.mac[2],
      station.mac[3], station.mac[4], station.mac[5]);

    currentDevices[currentCount++] = String(mac);
  }

  //  NEW / REJOIN
  for (int i = 0; i < currentCount; i++) {
    bool found = false;

    for (int j = 0; j < prevCount; j++) {
      if (currentDevices[i] == prevDevices[j]) {
        found = true;
        break;
      }
    }

    if (!found) {
      String mac = currentDevices[i];

      bool seenBefore = false;
      int index = -1;

      for (int k = 0; k < deviceCount; k++) {
        if (knownDevices[k] == mac) {
          seenBefore = true;
          index = k;
          break;
        }
      }

      if (!seenBefore && deviceCount < MAX_DEVICES) {
        knownDevices[deviceCount] = mac;
        deviceNames[deviceCount] = generateName(deviceCount);
        reconnectCount[deviceCount] = 0;
        index = deviceCount;
        deviceCount++;
      } else {
        reconnectCount[index]++;
      }

      connectTime[index] = millis();

      // 🔥 FRIENDLY / UNFRIENDLY PRINT
      if (isFriendly(mac)) {
        Serial.print("🟢 FRIENDLY DEVICE CONNECTED: ");
      } else {
        Serial.print("⚠️ UNRECOGNIZED DEVICE CONNECTED: ");
      }

      Serial.println(deviceNames[index]);
    }
  }

  //  DISCONNECT
  for (int i = 0; i < prevCount; i++) {
    bool stillHere = false;

    for (int j = 0; j < currentCount; j++) {
      if (prevDevices[i] == currentDevices[j]) {
        stillHere = true;
        break;
      }
    }

    if (!stillHere) {
      for (int k = 0; k < deviceCount; k++) {
        if (knownDevices[k] == prevDevices[i]) {

          unsigned long duration = (millis() - connectTime[k]) / 1000;

          Serial.print("DEVICE DISCONNECTED: ");
          Serial.println(deviceNames[k]);

          Serial.print("Duration: ");
          Serial.print(duration);
          Serial.println(" sec");

          Serial.print("Reconnects: ");
          Serial.println(reconnectCount[k]);

          Serial.println("----------------------");
        }
      }
    }
  }

  //  Update prev
  prevCount = currentCount;
  for (int i = 0; i < currentCount; i++) {
    prevDevices[i] = currentDevices[i];
  }

  //  PRINT ALL ACTIVE DEVICES EVERY LOOP
  Serial.println("\n=== ACTIVE DEVICES ===");

  for (int i = 0; i < currentCount; i++) {
    String mac = currentDevices[i];

    for (int k = 0; k < deviceCount; k++) {
      if (knownDevices[k] == mac) {

        tcpip_adapter_sta_info_t station = adapterList.sta[i];
        IPAddress ip = IPAddress(station.ip.addr);

        unsigned long duration = (millis() - connectTime[k]) / 1000;

        Serial.print("Device: ");

        // 🔥 Friendly Tag in Live List
        if (isFriendly(mac)) {
          Serial.print("[Friendly] ");
        } else {
          Serial.print("[Unknown] ");
        }

        Serial.print(deviceNames[k]);

        Serial.print(" | MAC: ");
        Serial.print(mac);

        Serial.print(" | IP: ");
        Serial.print(ip);

        Serial.print(" | Time: ");
        Serial.print(duration);
        Serial.print("s");

        Serial.print(" | Reconnects: ");
        Serial.print(reconnectCount[k]);

        Serial.println();
      }
    }
  }

  Serial.println("======================\n");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== HAWK - EYE ===");

  WiFi.mode(WIFI_AP_STA);

  connectToUpstream();

  WiFi.softAP(ssid, password);

  Serial.println("Initializing");
}

void loop() {
  printClients();
  delay(2000);
}