#pragma once

#include <Arduino.h>
#include <N2kMsg.h>


// Send to Yacht device clients over udp using the cast address
void GwSendYD(const tN2kMsg &N2kMsg);
