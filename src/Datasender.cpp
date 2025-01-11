// Demo: NMEA2000 library. Send different NMEA 2000 messages as example to the
// bus.
//
// MessageSender uses tN2kSyncScheduler, which sends all messages synchronized
// to library open time. As default messages will have their default(NMEA2000
// specified) period. Messages also has different offsets to avoid filling up
// buffer at once. NMEA2000 bus can handle about 2.5 frame/ms. So two single
// frame messages could be sent at same time, but e.g. 7 frame GNSS position
// data takes about 3 ms. With unlapping offsets there should be only one
// message on buffer at time
//
// If you need to test new message for testing, write sender function - see.
// e.g. SendN2kTemperatureExt around line 360 (may change after update). Then
// add function to N2kSendMessages (somewhere line 504 ) vector in similar way
// as SendN2kTemperatureExt. Set message period value according to message
// definition - most periods has been listed also on NMEA2000.cpp.
//
// Messages sending will be done by SendN2kMessages, which loops through message
// sender vector and sets loop NextSend scheduler to next nearest sent time. In
// this way function does not need every time loop through all messages. When it
// is time to send some messages, it loops them, sends necessary messages and
// sets NextSend to next nearest time. In ESP32 message sending could be own
// task and NextSend could be used just delay to leave time for other tasks.
//
// MessageSender has simple command line interface. With simple commands you
// e.g. disable all messages and then enable them one by one. To get available
// commands type ? and line feed to console to get help.

#include "ESPmDNS.h"
#include <Arduino.h>
#include <DNSServer.h>
#include <StringStream.h>
#include <WebServer.h>

#include <SysInfo.h>
#include <html_footer.h>
#include <html_header.h>

#include <display.h>
#include <ntpclient.h>
#include <time.h>
#include <touch_calibrate.h>

// WiFi handling
#include <MyWiFi.h>

// Yacht data sender
#include <N2ktoYD.h>

// N2k message generator
#include <n2ksender.h>

// Shell
#include <GwShell.h>

// telnet client
#include <GwTelnet.h>

#include <MyMdns.h>

// system preferences stored in flash
#include <GwPrefs.h>

// Rotary encoder
#include <rotary_encoder.h>

// N2k message decoder
#include <n2kdatadecode.h>

// Forward declarations for functions
void CheckCommand();

// Global objects and variables
String host_name;
String Model = "Naiad N2K Sender";

// Define the console to output to serial at startup.
// this can get changed later, eg in the gwshell.
Stream *Console = &Serial;

void prompt() {
    printSummary(1, NULL);
    Serial.printf("?<lf> for help. >");
}

uint8_t chipid[6];
String macAddress;

#define Max_YD_Message_Size 500
char YD_msg[Max_YD_Message_Size] = "";

// Create UDP instance
WiFiUDP udp;

// The telnet server for the shell.
WiFiServer telnetServer(23);

// Touch calibration. If true then run the touch calibration once
// this stores the results in nvram and the next boot will use those values
bool runCalibration = false;

// *****************************************************************************
void setup() {
    Serial.begin(115200);
    delay(2000); // Give a bit time for console to be ready
                 // get the MAC address
    Serial.printf("Setting up\n");

    GwPrefsInit();

    runCalibration = GwGetInt(TFTCALIB);

    if (runCalibration) {
        // Create a tft object for the touch calibration
        TFT_eSPI tft(TFT_HEIGHT, TFT_WIDTH);

        // Initialise the TFT screen
        tft.init();

        // Set the rotation before we calibrate
        tft.setRotation(0);

        // Calibrate the touch screen and retrieve the scaling factors
        touch_calibrate(tft);

        // reset th request so we dont try next time
        GwSetInt(TFTCALIB, 0);
        
        Serial.printf("Calibration done, rebooting...");
        Serial.flush();
        delay(5000);

        // And reset
        ESP.restart();
    }

    uint8_t chipid[6];
    uint32_t id = 0;

    // Initialize serial port.
 
    esp_err_t fuse_error = esp_efuse_mac_get_default(chipid);
    if (fuse_error) {
        Serial.printf("efuse error: %d\n", fuse_error);
    }

    for (uint16_t i = 0; i < 6; i++) {
        if (i != 0) {
            macAddress += ":";
        }
        id += (chipid[i] << (7 * i));
        macAddress += String(chipid[i], HEX);
    }
     // Generate the hostname by appending the last two octets of the mac address
    // to make it unique
    String hname = "n2ktester";
    host_name = hname + String(chipid[4], HEX) + String(chipid[5], HEX);

    setupN2ksender();

    setupDecoder(Console);

    // Init the display - this requires the n2k to have been initialised
    displayInit();

    // Start the wifi
    wifiSetup(host_name);

    // Init Mdns - if enabled
    initMdns(host_name);

    // Register the services
    addMdnsService("telnet", "tcp",
                   23); // Telnet server of RemoteDebug, register as telnet

    // Start the telnet server
    telnetServer.begin();

    // Setup the ntp client
    ntp_setup();

    // Get the ntp time
    time_t epoch = ntp_epoch();

    struct tm *now = gmtime(&epoch);
    Serial.printf("%02d:%02d:%02d\n", now->tm_hour, now->tm_min, now->tm_sec);

    // Adjust the local clock
    struct timeval tv;
    tv.tv_sec = epoch;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);

    // Init the interactive shell using the serial connection
    initGwShell();
    setShellSource(&Serial);

    rotary_setup();
}

// *****************************************************************************
void loop() {
    if (!runCalibration) {
        handleN2kMessages();

        wifiWork();
        wifiCheck();

        // Update the display and handle touch events
        handleDisplay();

        // handle the telnet session
        handleTelnet();

        // And run and shell commands
        handleShell();
    }
}
