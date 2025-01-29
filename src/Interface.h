// A class that holds informatioj on a n2k interface.
// This could be a physical CAN port or a UDP Yacht Data format

#pragma once

#include <Arduino.h>
#include <N2kMessages.h>
#include <map>

class pgnCount {
    public:
        int pgn;
        int count;
};

class PgnMsg {
    public:
        int pgn;
        tN2kMsg msg;
};


class Interface {
    public:

    // Map for received n2k message counts. Logs the PGN and the count
    std::map <int, int> messageCounts;

    // Iterator for the counts
    std::map <int, int>::iterator countIt;

    // Map for the latest message of a given PGN
    std::map <int, tN2kMsg> lastMessage;

    // Iterator for the last saved messages
    std::map <int, tN2kMsg>::iterator lastIt;

    String name;

    Interface(const char *);

    uint32_t count();
    uint32_t addMsg(const tN2kMsg & msg);
    void showMessageCounts(Stream &);
    void countBegin();
    bool countNext(pgnCount &);
    void msgBegin();
    bool msgNext(PgnMsg &);
    bool findLastMessage(int, PgnMsg &);
};

// Class to map between PGN and the name
class PgnToStr {
    public:
    PgnToStr();

    // return a pointer to the name for the pgn, or empty string if not found
    const char * getStr(int pgn);
    private:
        std::map<long, const char *> pgnToStr;
};