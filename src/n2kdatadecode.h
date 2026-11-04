#include <NMEA2000.h>

void setupDecoder(Stream * s);
//NMEA 2000 message handler
void HandleNMEA2000Msg(const tN2kMsg &N2kMsg, Stream * stream);