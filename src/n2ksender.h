#pragma once

#define MAX_NMEA2000_MESSAGE_SEASMART_SIZE 1024

// N2K sender preference keys
#define YDENABLE    "ydenable"
#define CANENBALE   "canenable"
#define N2KMAP      "n2kmap"

void setupN2ksender();
void handleN2kMessages();
int ToggleStats(int, char**);
int ToggleSending(int, char**);
int ToggleShowSentMessages(int, char**);
int ToggleForward(int, char**);
int ClearStatistics(int, char**);
int ToggleMsgSending(int, char**Params);
int ToggleYD(int, char**params);
int SendMessageNow(int, char**Params);
int SetMessagePeriod(int, char**Params);
int SetMessageOffset(int, char**Params);
int EnableAllMessages(int, char**);
int DisableAllMessages(int, char**);
int ListMessages(int, char**);
int printSummary(int, char**);




