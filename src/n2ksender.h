#pragma once

#include <NMEA2000.h>

#define MAX_NMEA2000_MESSAGE_SEASMART_SIZE 1024

// N2K sender preference keys
#define YDENABLE    "ydenable"
#define YDRXENABLE  "ydrxenable"
#define CANENBALE   "canenable"

#define N2KMAP      "n2kmap"
#define MAXN2KMESSAGES 32   // max number of messages we can send. This could be dangerous if we decide to add any to the n2ksender

using tN2kSendFunction = void (*)();

// Structure for holding message sending information
struct tN2kSendMessage {
    tN2kSendFunction SendFunction;
    const char *const Description;
    tN2kSyncScheduler Scheduler;

    tN2kSendMessage(tN2kSendFunction _SendFunction, const char *const _Description, uint32_t /* _NextTime */
                    ,
                    uint32_t _Period, uint32_t _Offset, bool _Enabled) : SendFunction(_SendFunction),
                                                                         Description(_Description),
                                                                         Scheduler(_Enabled, _Period, _Offset) {}
    void Enable(bool state);
};

extern tN2kSendMessage N2kSendMessages[];
extern size_t nN2kSendMessages;

void setupN2ksender();
void handleN2kMessages();
int ToggleStats(int, char**);
int ToggleSending(int, char**);
int ToggleShowSentMessages(int, char**);
int ToggleForward(int, char**);
int ClearStatistics(int, char**);
int ToggleMsgSending(int, char**Params);
int ToggleYD(int, char**params);
int ToggleYDRX(int, char**params);
int SendMessageNow(int, char**Params);
int SetMessagePeriod(int, char**Params);
int SetMessageOffset(int, char**Params);
int EnableAllMessages(int, char**);
int DisableAllMessages(int, char**);
int ListMessages(int, char**);
int printSummary(int, char**);

void ListDevices(Stream &stream, bool force);

// save the map as a preference
void saveMap();
// Create the map and save it in enableMap
void msgToMap();
extern uint32_t enableMap;




