// System and net info

/*
Copyright (c) 2022 Peter Martin www.naiadhome.com

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <Arduino.h>
//#include <ESP.h>
#include <GwPrefs.h>
#include <NMEA0183Messages.h>
#include <N2kMessages.h>
#include <SysInfo.h>
#include <esp_wifi.h>
#include <n2kdatadecode.h>
#include "uptime_formatter.h"
#include <StringStream.h>
#include <display.h>

void getNetInfo(Stream &s) {
    wifi_sta_list_t wifi_sta_list;
    tcpip_adapter_sta_list_t adapter_sta_list;

    s.println("=========== NETWORK ==========");
    s.printf("HOST NAME: %s\n", host_name.c_str());
    s.printf("MAC: %s\n", macAddress.c_str());
    s.printf("WifiMode %s\n", WifiMode.c_str());
    s.printf("WifiIP %s\n", WifiIP.c_str());
    s.printf("WifiSSID %s\n", WifiSSID.c_str());

    // Get the wifi station list and list the connected device details
    memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
    memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));
    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);

    for (int i = 0; i < adapter_sta_list.num; i++) {
        tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];

        s.printf("Station number %d \n", i);
        s.printf("MAC: ");
        for (int j = 0; j < 6; j++) {
            s.printf("%02X", station.mac[j]);
            if (i < 5) s.print(":");
        }
        s.printf("\nIP: ");
        s.println(ip4addr_ntoa((const ip4_addr_t *)&(station.ip)));
        s.println("");
    }

    s.println("=========== END ==========");
}

void getSysInfo(Stream &s) {
    EspClass esp;

    uint32_t heap = esp.getHeapSize();      // total heap size
    uint32_t freeheap = esp.getFreeHeap();  // available heap
    uint32_t heapUsedPc = (heap - freeheap) * 100 / heap;

    uint8_t chiprev = esp.getChipRevision();
    const char *model = esp.getChipModel();
    uint32_t sketchSize = esp.getSketchSize();
    uint32_t freeSketch = esp.getFreeSketchSpace();
    uint32_t flashsize = esp.getFlashChipSize();
    uint32_t flashUsedPc = (flashsize - freeSketch) * 100 / flashsize;
    uint64_t efuse = esp.getEfuseMac();
    String uptime = uptime_formatter::getUptime();
    String node = GwGetVal(LASTNODEADDRESS);

    s.println("=========== SYSTEM ==========");
    s.printf("Model %s\n", Model.c_str());
    s.printf("Node: %s\n", node.c_str());
    s.printf("Uptime: %s", uptime.c_str());
    s.printf("Heap \t%d\n", heap);
    s.printf("Heap Free\t%d\n", freeheap);
    s.printf("Heap used %d%%\n", heapUsedPc);
    s.printf("ChipRev \t%d\n", chiprev);
    s.printf("Model \t%s\n", model);
    s.printf("Sketch \t%d\n", sketchSize);
    s.printf("Sketch Free \t%d\n", freeSketch);
    s.printf("Flash used %d%%\n", flashUsedPc);
    s.printf("Efuse \t0x%llx\n", efuse);
    for (int c = 0; c < 2; c++) {
        s.printf("CPU %d load %d%%\n", c, getCpuAvg(c));
    }
    s.println("=========== SETTINGS ==========");
    GwPrint(s);
    s.println("=========== END ==========");
}

extern std::map<int, int> N2kMsgMap;
void getN2kMsgs(Stream &s) {
    std::map<int, int>::iterator it = N2kMsgMap.begin();

    s.println("======== N2K Messages ====");

    while (it != N2kMsgMap.end()) {
        s.printf("%d %d\n", it->first, it->second);
        it++;
    }
    s.println("=========== END ==========");
}

// Find and print a PGN
extern std::map<int, tN2kMsg> N2kLastMessage;
std::map<int, tN2kMsg>::iterator iter;

void printPGN(unsigned long PGN) {
    iter = N2kLastMessage.find(PGN);
    if(iter == N2kLastMessage.end()) {
        Serial.printf("No messages with PGN %ld found\n", PGN);
    } else {
        StringStream output;
        Serial.printf("Message with PGN %ld found\n", PGN);
        HandleNMEA2000Msg(iter->second, & output);
        updateTextArea(SCR_CAN, output.data);
        Serial.printf("%s\n", output.data.c_str());
    }
}