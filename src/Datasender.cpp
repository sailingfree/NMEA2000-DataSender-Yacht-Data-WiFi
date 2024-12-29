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

// Forward declarations for functions
void CheckCommand();

// Global objects and variables
String host_name;
String Model = "Naiad N2K Sender";

// Define the console to output to serial at startup.
// this can get changed later, eg in the gwshell.
Stream *Console = &Serial;

void prompt() {
    printSummary();
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

// *****************************************************************************
void setup() {
    uint8_t chipid[6];
    uint32_t id = 0;

    // Initialize serial port.
    Serial.begin(115200);
    delay(2000); // Give a bit time for console to be ready
                 // get the MAC address
    Serial.printf("Setting up\n");
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
    GwPrefsInit();

    // Generate the hostname by appending the last two octets of the mac address
    // to make it unique
    String hname = "n2ktester";
    host_name = hname + String(chipid[4], HEX) + String(chipid[5], HEX);

   // Init the display
    displayInit(); 
 
    // Start the wifi
    wifiSetup(host_name);

    // Init Mdns - if enabled
    initMdns(host_name);

    // Register the services
    addMdnsService("telnet", "tcp", 23);  // Telnet server of RemoteDebug, register as telnet

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
    setupN2ksender();

    // Init the interactive shell
    initGwShell();
    setShellSource(&Serial);

//    prompt();
}

// *****************************************************************************
void loop() {
    handleN2kMessages();

//    CheckCommand();
    wifiWork();
    wifiCheck();

    // Update the display and handle touch events
    handleDisplay();
    
    // handle the telnet session
    handleTelnet();

    // And run and shell commands
   handleShell();
}

using tCommandFunction = void (*)(const char *Params);

struct tCommand {
    const char *const Command;
    const char *const Usage;
    const char *const Description;
    tCommandFunction CommandFunction;
};

void ListCommands(const char *Command);

tCommand Commands[] = {
    {"?", 0, "List commands", ListCommands},
    {"p", 0, "Toggle sending", ToggleSending},
    {"stat", 0, "Toggle stat printing", ToggleStats},
    {"cl", 0, "Clear statistics", ClearStatistics},
    {"e", 0, "Enable all messages", EnableAllMessages},
    {"d", 0, "Disable all messages", DisableAllMessages},
    {"s", 0, "Toggle show sent messages", ToggleShowSentMessages},
    {"fwd", 0, "Toggle show other bus messages", ToggleForward},
    {"msgs", 0, "List messages", ListMessages},
    {"msg", "msg <x>", "Toggle message x sending", ToggleMsgSending},
    {"once", "once <x>", "Sends message x on request", SendMessageNow},
    {"yd", 0, "Toggle sending YD broadcast UDP data", ToggleYD},
    {"period", "period <x> [p]",
     "Set message x period to p. If p is not defined, just shows period.",
     SetMessagePeriod},
    {"offset", "offset <x> [o]",
     "Set message x offset to o. If o is not defined, just shows offset.",
     SetMessageOffset}};

size_t nCommands = sizeof(Commands) / sizeof(tCommand);

// *****************************************************************************
void ListCommands(const char *Command) {
    Serial.println();
    Serial.println("Commands:");
    char Info[200];
    for (size_t i = 0; i < nCommands; i++) {
        strcpy(Info, "  ");
        strcat(Info, Commands[i].Usage != 0 ? Commands[i].Usage
                                            : Commands[i].Command);
        size_t len = strlen(Info);
        if (len < 16) {
            memset(Info + len, ' ', 16 - len);
            Info[16] = 0;
        }
        strcat(Info, "- ");
        strcat(Info, Commands[i].Description);
        Serial.println(Info);
    }
    Serial.println();
    prompt();
}

// *****************************************************************************
void CheckCommand() {
    static char Command[20] = {0};
    static size_t CommandPos = 0;

    if (Serial.available()) {
        char ch = Serial.read();
        if (ch == '\n') {
            char *Params = strchr(Command, ' ');
            bool FoundCommand = false;
            if (Params != 0) {
                *Params = 0;
                Params++;
            }
            for (size_t i = 0; i < nCommands; i++) {
                if (strcmp(Command, Commands[i].Command) == 0) {
                    Commands[i].CommandFunction(Params);
                    FoundCommand = true;
                    break;
                }
            }
            if (!FoundCommand) {
                Serial.print("Unknown command ");
                Serial.println(Command);
                prompt();
            }
            CommandPos = 0;
            Command[CommandPos] = 0;
        } else {
            Serial.printf("%c", ch);
            if (ch >= ' ' && CommandPos + 1 < sizeof(Command)) {
                Command[CommandPos] = ch;
                CommandPos++;
                Command[CommandPos] = 0;
            }
        }
    }
}
