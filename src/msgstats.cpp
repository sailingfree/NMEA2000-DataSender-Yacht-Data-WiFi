#include <Interface.h>

// Global message stats
Interface canRX("CAN Messages Recieved");
Interface canTX("CAN Messages Transmitted");
Interface ydRX("YD Messages Recieved");
Interface ydTX("YD Messages Transmitted");

// n2k message pgn to string
PgnToStr pgnToStr;