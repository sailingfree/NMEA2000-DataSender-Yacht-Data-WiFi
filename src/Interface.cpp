//

#include <Interface.h>

Interface::Interface(const char * title) {
    name = title;
}

bool Interface::findLastMessage(int pgn, PgnMsg &msg) {
    bool result = false;
    std::map<int, tN2kMsg>::iterator iter;

    iter = lastMessage.find(pgn);
    if (iter == lastMessage.end()) {
        Serial.printf("No messages with PGN %d found\n", pgn);
    } else {
        msg.pgn = pgn;
        msg.msg = iter->second;
        result = true;
    }
    return result;
}

// Add a message to the stats.
// Increaments the count and saves this message as the last message
uint32_t Interface::addMsg(const tN2kMsg & msg) {
    int pgn = msg.PGN;
    tN2kMsg msgCopy = msg;

    messageCounts[pgn]++;
    lastMessage[pgn] = msgCopy;
    return 0;
}

// Initialise the itertator for the coun map
void Interface::countBegin() {
  countIt = messageCounts.begin();
}

// Next function for the counts
bool Interface::countNext(pgnCount & out) {
    bool result = false;
    if(countIt != messageCounts.end()) {
        result = true;
        out.pgn = countIt->first;
        out.count = countIt->second;
        countIt++;
    }
   return result;
}

// Initialise the itertator for the message map
void Interface::msgBegin() {
  lastIt = lastMessage.begin();
}

// Next function for the message map
bool Interface::msgNext(PgnMsg & out) {
    bool result = false;
    if(lastIt != lastMessage.end()) {
        result = true;
        out.pgn = lastIt->first;
        out.msg = lastIt->second;
        lastIt++;
    }
   return result;
}

// The total number of elements in the count map
uint32_t Interface::count() {
    uint32_t total = 0;
    pgnCount msg;
    countBegin();
    while(countNext(msg)) {
        total += msg.count;
    }

    return total;
}

///////////////////////////////////////////////////////////////////////////////
// pgntostr members

PgnToStr::PgnToStr() {
    pgnToStr = {
        {126992L, "SystemTime"},     
        {127245L, "Rudder"},
        {127250L, "Heading"},        
        {127257L, "Attitude"},
        {127488L, "EngineRapid"},    
        {127489L, "EngineDynamic"},
        {127493L, "Transmission"},   
        {127497L, "TripFuelConsumption"},
        {127501L, "BinaryStatus"},   
        {127505L, "FluidLevel"},
        {127506L, "DCStatus"},       
        {127513L, "BatteryConfig"},
        {128259L, "Speed"},          
        {128267L, "WaterDepth"},
        {129026L, "COGSOG"},         
        {129029L, "GNSS"},
        {129033L, "LocalOffset"},    
        {129045L, "UserDatum"},
        {129540L, "GNSSSatsInView"}, 
        {130310L, "OutsideEnvironmental"},
        {130312L, "Temperature"},    
        {130313L, "Humidity"},
        {130314L, "Pressure"},       
        {130316L, "TemperatureExt"},
    };
}

// Get the pgn name from the map
// return empty string if not found
const char *PgnToStr::getStr(int pgn) {
    // hand rolled find() equivalent
    const char * result = "";
    std::map<long, const char *>::iterator it2;
    it2 = pgnToStr.begin();
    bool found = false;
    while (!found && it2 != pgnToStr.end()) {
        if (it2->first == pgn) {
            result = it2->second;
            found = true;  // stop loop
        }
        it2++;
    }
    return result;
}