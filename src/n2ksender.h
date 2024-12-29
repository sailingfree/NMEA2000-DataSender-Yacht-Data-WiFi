#pragma once

#define MAX_NMEA2000_MESSAGE_SEASMART_SIZE 1024

void setupN2ksender();
void handleN2kMessages();
void ToggleStats(const char *);
void ToggleSending(const char *);
void ToggleShowSentMessages(const char *);
void ToggleForward(const char *);
void ClearStatistics(const char *);
void ToggleMsgSending(const char *Params);
void ToggleYD(const char *params);
void SendMessageNow(const char *Params);
void SetMessagePeriod(const char *Params);
void SetMessageOffset(const char *Params);
void EnableAllMessages(const char *);
void DisableAllMessages(const char *);
void ListMessages(const char *);
void printSummary();




