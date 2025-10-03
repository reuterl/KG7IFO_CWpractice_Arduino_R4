#include "api/IPAddress.h"
#include "CWcommUDP.h"
#include <string.h>
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
#include "arduino_secrets.h"

#define SPRINTF(FMT, args...) \
  sprintf(sSprintf, FMT, args); \
  Serial.print(sSprintf);
extern char sSprintf[];

void CWcommUDP::constructorCommon(void) {
  localPort = 2390;
  status = WL_IDLE_STATUS;
  keyIndex = 0;
  timeoutIP = 10000;  // 10 second timeout for IP address
  strcpy(ssid, SECRET_SSID);
  strcpy(pass, SECRET_PASS);
}

CWcommUDP::CWcommUDP(machinedetat* _MDE) {
  MDE = _MDE;
  constructorCommon();
}

CWcommUDP::CWcommUDP(uint16_t _localPort, machinedetat* _MDE) {
  constructorCommon();
  localPort = _localPort;
  MDE = _MDE;
}

CWcommUDP::~CWcommUDP() {}

void CWcommUDP::setupUDP(void) {
  uint64_t startTime = millis();
  bool wifiConnected = false;
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("\nPlease upgrade the firmware\n");
  }

  while (!wifiConnected) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    if (status == WL_CONNECTED) {
      wifiConnected = true;
    } else {
      delay(500);
    }
  }

  Serial.println("\nCW Serrver Starting. Awaiting connection from Python client...");
  Udp.begin(localPort);

  Serial.println("=============================");
  Serial.print("Connected to WiFi. Time = ");
  Serial.print(millis() - startTime);
  Serial.println("ms.");

  // wait for gateway to assign us an IP address.
  wifiConnected = false;
  startTime = millis();
  while (!wifiConnected) {
    IPAddress ip = WiFi.localIP();
    if (ip == IPAddress("0.0.0.0")) {
      if ((millis() - startTime) > timeoutIP) {
        Serial.println("\nFailed to get IP address!\n");
        while (true)
          ;  // Hang.
      }
      delay(500);
    } else {
      wifiConnected = true;
    }
  }

  printWifiStatus();
  Serial.println("=============================");
}

void CWcommUDP::printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void CWcommUDP::writePacket(uint8_t * Msg, uint8_t Length){
  //Serial.println("writePacket()");
  
  //Serial.print("Udp.remoteIP: ");
  //Serial.println(Udp.remoteIP());
  //Serial.print("Udp.remotePort: ");
  //Serial.println(Udp.remotePort());

  Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
  Udp.write(Msg, Length);
  Udp.endPacket();
}

char CWcommUDP::udpRcvStream(void) {
  // if there's data available, read a packet
  packetSize = Udp.parsePacket();
  if (packetSize) {
    Serial.print("\nReceived packet of size ");
    Serial.println(packetSize);
    Serial.print("From ");
    remoteIp = Udp.remoteIP();
    Serial.print(remoteIp);
    Serial.print(", port ");
    Serial.println(Udp.remotePort());

    // read the packet into packetBuffer
    int len = Udp.read(packetBuffer, sizeof(packetBuffer));
    //Serial.print("Udp.read, len = ");
    //Serial.println(len);
    for (int x = 0; x < len; x++) {
      //sprintf(sSprintf, "packetBuffer[%d] = 0x%02X\n", x, packetBuffer[x]);
      //Serial.print(sSprintf);
      MDE->msgParser(packetBuffer[x]);
    }
  }
}
