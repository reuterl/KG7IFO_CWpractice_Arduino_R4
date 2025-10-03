#ifndef CWCOMMUDP_H_
#define CWCOMMUDP_H_
#include <stdint.h>
#include <math.h>
#include <WiFiS3.h>
#include "machinedetat.h"


class CWcommUDP {
private:
  int status;
  char ssid[16];       // your network SSID (name)
  char pass[16];       // your network password (use for WPA, or use as key for WEP)
  int keyIndex;        // your network key index number (needed only for WEP)
  uint16_t localPort;  // local port to listen on
  uint64_t timeoutIP;
  WiFiUDP Udp;
  uint16_t packetSize;
  IPAddress remoteIp;
  uint8_t packetBuffer[256];  //buffer to hold incoming packet
  machinedetat* MDE;

private:
  void constructorCommon(void);
  void printWifiStatus();
public:
  CWcommUDP(machinedetat* _MDE);
  CWcommUDP(uint16_t _localPort, machinedetat* _MDE);
  virtual ~CWcommUDP();
  void setupUDP(void);
  void writePacket(uint8_t * Msg, uint8_t Length);
  char udpRcvStream(void);
};

#endif