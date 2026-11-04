/*

 Udp NTP Client

 Get the time from a Network Time Protocol (NTP) time server
 Demonstrates use of UDP sendPacket and ReceivePacket
 For more on NTP time servers and the messages needed to communicate with them,
 see http://en.wikipedia.org/wiki/Network_Time_Protocol

 created 4 Sep 2010
 by Michael Margolis
 modified 9 Apr 2012
 by Tom Igoe
 modified 02 Sept 2015
 by Arturo Guadalupi

 This code is in the public domain.

 */

#include <MyWiFi.h>
#include <WiFiUdp.h>

const char timeServer[] = "0.pool.ntp.org"; // NTP server

const int NTP_PACKET_SIZE =
    48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing
                                    // packets

static const unsigned int localPort =
    8888; // local port to listen for UDP packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;

// Forward declaration
static void sendNTPpacket(const char *address);

// Setup the ntp client
void ntp_setup() {
    if(wifiConnected()) {
        Udp.begin(localPort);
    }
}

time_t ntp_epoch() {
    time_t epoch;
    if (!wifiConnected()) {
        epoch = time(NULL);
    } else {
        sendNTPpacket(timeServer); // send an NTP packet to a time server

        // wait to see if a reply is available
        delay(100);
        if (Udp.parsePacket()) {
            // We've received a packet, read the data from it
            Udp.read(packetBuffer,
                     NTP_PACKET_SIZE); // read the packet into the buffer

            // the timestamp starts at byte 40 of the received packet and is
            // four bytes, or two words, long. First, extract the two words:

            time_t highWord = word(packetBuffer[40], packetBuffer[41]);
            time_t lowWord = word(packetBuffer[42], packetBuffer[43]);
            // combine the four bytes (two words) into a long integer
            // this is NTP time (seconds since Jan 1 1900):
            time_t secsSince1900 = highWord << 16 | lowWord;
            Serial.print("Seconds since Jan 1 1900 = ");
            Serial.println(secsSince1900);

            // now convert NTP time into everyday time:
            Serial.print("Unix time = ");
            // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
            const uint64_t seventyYears = 2208988800UL;
            // subtract seventy years:
            epoch = secsSince1900 - seventyYears;
            // print Unix time:
            Serial.println(epoch);

            // print the hour, minute and second:
            Serial.print("The UTC time is "); // UTC is the time at Greenwich
                                              // Meridian (GMT)
            Serial.print((epoch % 86400L) /
                         3600); // print the hour (86400 equals secs per day)
            Serial.print(':');
            if (((epoch % 3600) / 60) < 10) {
                // In the first 10 minutes of each hour, we'll want a leading
                // '0'
                Serial.print('0');
            }
            Serial.print((epoch % 3600) /
                         60); // print the minute (3600 equals secs per minute)
            Serial.print(':');
            if ((epoch % 60) < 10) {
                // In the first 10 seconds of each minute, we'll want a leading
                // '0'
                Serial.print('0');
            }
            Serial.println(epoch % 60); // print the second
        } else {
            epoch = time(NULL); // No response so use local time
        }
    }
    return epoch;
}

// send an NTP request to the time server at the given address
static void sendNTPpacket(const char *address) {
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011; // LI, Version, Mode
    packetBuffer[1] = 0;          // Stratum, or type of clock
    packetBuffer[2] = 6;          // Polling Interval
    packetBuffer[3] = 0xEC;       // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    Udp.beginPacket(address, 123); // NTP requests are to port 123
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    Udp.endPacket();
}
