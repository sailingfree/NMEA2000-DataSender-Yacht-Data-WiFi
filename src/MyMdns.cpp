#include "ESPmDNS.h"
#include <Arduino.h>
#include <GwDefs.h>
#include <MyMdns.h>
#include <MyWiFi.h>

// Register host name in mDNS
void initMdns(String &host_name) {
    if (wifiConnected()) {
        if (MDNS.begin(host_name.c_str())) {
            Console->print("* MDNS responder started. Hostname -> ");
            Console->println(host_name);
        }
    } else {
        Serial.printf("Wifi not connected, not starting mdns\n");
    }
}

void addMdnsService(const char *service, const char *proto, uint16_t port) {
    if (wifiConnected()) {
        Console->printf("Adding service %s to MDNS\n", service);
        MDNS.addService(service, proto, port);
    }
}
