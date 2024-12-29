// Define all common globals
#pragma once

#include <Arduino.h>
#include <BoatData.h>
#include <N2kMsg.h>
#include <WiFi.h>
#include <YDtoN2kUDP.h>
#include <NMEA2000.h>
#include <NMEA0183.h>
#include <map>

extern Stream* Console;
extern tBoatData boatData;
extern YDtoN2kUDP YDRecvUDP;
extern tNMEA2000& NMEA2000;
extern String hostName;
extern tNMEA0183 AIS_NMEA0183;
extern uint16_t daysSince1970;
extern double secondsSinceMidnight;

extern std::map<String, String> mapGps;
extern std::map<String, String> mapSensors;
extern std::map<int, int> mapN2kMsg;
extern std::map<int, int> mapYdMsg;


extern String wifiMode, wifiIP, wifiSSID;
extern WiFiServer telnetServer;
extern WiFiServer jsonServer;
extern String hostName, macAddress;

extern int nodeAddress;
extern String hostName;
extern String modelName;
extern float temp;
extern float voltage;

extern uint32_t canMsgCount, ydMsgCount;
extern uint32_t thisNodeId;

