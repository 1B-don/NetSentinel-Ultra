#ifndef NETSENTINEL_AP_PARSER_H
#define NETSENTINEL_AP_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define NS_SSID_SIZE 33
#define NS_BSSID_SIZE 18

/* A parsed, receive-only Wi-Fi access-point observation. */
typedef struct {
    char ssid[NS_SSID_SIZE];
    char bssid[NS_BSSID_SIZE]; /* empty when Marauder omits BSSID */
    int16_t rssi;
    uint8_t channel;
} NsAccessPoint;

/* Best-effort parser for verified Marauder scanap and list -a line formats. */
bool ns_parse_ap_line(const char* line, NsAccessPoint* out);

#endif
