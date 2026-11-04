#include <Arduino.h>
#include <N2kMsg.h>
#include <Seasmart.h>
#include <BoatData.h>
#include <WiFi.h>
#include <YDtoN2kUDP.h>
#include <NMEA2000.h>
#include <GwDefs.h>

#define MAX_NMEA2000_MESSAGE_SEASMART_SIZE 500

// UPD broadcast for Navionics, OpenCPN, etc.
// We listenon this port
static const int YDudpPort = 4444;  // port 4444 is for the Yacht devices interface

// Create UDP instance for sending YD messages
WiFiUDP     YDSendUDP;

// The buffer to construct YD messages
#define Max_YD_Message_Size 500
static char YD_msg[Max_YD_Message_Size] = "";

/**
 * @name: N2kToYD_Can
 */
void N2kToYD_Can(const tN2kMsg &msg, char *MsgBuf) {
    uint16_t DaysSince1970;
    double SecondsSinceMidnight;

    time_t now = time(NULL);
    struct tm *tm;
    tm = gmtime(&now);

    DaysSince1970 = now / (24 * 60 * 60);
    SecondsSinceMidnight = tm->tm_sec + tm->tm_min * 60 + tm->tm_hour * 60 * 60;

    int i, len;
    uint32_t canId = 0;
    char time_str[20];
    char Byte[5];
    unsigned int PF;
    time_t rawtime;
    struct tm ts;
    len = msg.DataLen;
    if (len > 134) {
        len = 134;
        Console->printf("Truncated from %d to 134\n", len);
    }

    // Set CanID

    canId = msg.Source & 0xff;
    PF = (msg.PGN >> 8) & 0xff;

    if (PF < 240) {
        canId = (canId | ((msg.Destination & 0xff) << 8));
        canId = (canId | (msg.PGN << 8));
    } else {
        canId = (canId | (msg.PGN << 8));
    }

    canId = (canId | (msg.Priority << 26));

    rawtime = (DaysSince1970 * 3600 * 24) + SecondsSinceMidnight;  // Create time from GNSS time;
    ts = *localtime(&rawtime);
    strftime(time_str, sizeof(time_str), "%T.000", &ts);  // Create time string
#pragma GCC diagnostic ignored "-Wformat-truncation"
    snprintf(MsgBuf, 25, "%s R %.8x", time_str, canId);  // Set time and canID

    for (i = 0; i < len; i++) {
        snprintf(Byte, 4, " %.2x", msg.Data[i]);  // Add data fields
        strcat(MsgBuf, Byte);
    }
}

// Send to Yacht device clients over udp using the broadcast address
void GwSendYD(const tN2kMsg &N2kMsg) {
    IPAddress udpAddress = WiFi.broadcastIP();
    N2kToYD_Can(N2kMsg, YD_msg);             // Create YD message from PGN
    YDSendUDP.beginPacket(udpAddress, YDudpPort);  // Send to UDP
    YDSendUDP.printf("%s\r\n", YD_msg);
    YDSendUDP.endPacket();

    // Add this to the ydTx messages
    

    char buf[MAX_NMEA2000_MESSAGE_SEASMART_SIZE];
    if (N2kToSeasmart(N2kMsg, millis(), buf, MAX_NMEA2000_MESSAGE_SEASMART_SIZE) == 0) {
        return;
    }
}

