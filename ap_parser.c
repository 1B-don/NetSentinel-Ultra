#include "ap_parser.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* This module has no Flipper SDK dependencies; it can be tested on a desktop. */
static const char* ns_skip_space(const char* p) {
    while(*p && isspace((unsigned char)*p)) p++;
    return p;
}

static bool ns_parse_int(const char* p, long* value) {
    if(!p) return false;
    p = ns_skip_space(p);
    if(!*p) return false;
    char* end = NULL;
    long number = strtol(p, &end, 10);
    if(end == p) return false;
    *value = number;
    return true;
}

static void ns_copy_ssid(char* dst, size_t dst_size, const char* start, const char* end) {
    while(start < end && isspace((unsigned char)*start)) start++;
    while(end > start && isspace((unsigned char)end[-1])) end--;
    size_t n = (size_t)(end - start);
    if(n >= dst_size) n = dst_size - 1;
    for(size_t i = 0; i < n; i++) {
        unsigned char ch = (unsigned char)start[i];
        dst[i] = (ch >= 32 && ch <= 126) ? (char)ch : '?';
    }
    dst[n] = 0;
    if(n == 0) {
        const char* name = "<hidden>";
        size_t m = strlen(name);
        if(m >= dst_size) m = dst_size - 1;
        memcpy(dst, name, m);
        dst[m] = 0;
    }
}

static bool ns_is_mac(const char* s) {
    for(size_t i = 0; i < 17; i++) {
        if(!s[i]) return false;
        if(i % 3 == 2) {
            if(s[i] != ':') return false;
        } else if(!isxdigit((unsigned char)s[i])) {
            return false;
        }
    }
    return !isxdigit((unsigned char)s[17]);
}

static const char* ns_find_mac(const char* s) {
    for(const char* p = s; *p; p++) {
        if(ns_is_mac(p)) return p;
    }
    return NULL;
}

bool ns_parse_ap_line(const char* line, NsAccessPoint* out) {
    if(!line || !out) return false;
    memset(out, 0, sizeof(*out));
    long ch = 0;
    long rssi = 0;

    /* Firmware versions may emit e.g.:
     * > RSSI: -57 Ch: 3 BSSID: 50:ff:20:84:d6:0f ESSID: Octoglass
     * -59 Ch: 3 00:11:22:33:44:55 ESSID: Example
     */
    const char* ssid_label = strstr(line, "ESSID:");
    size_t ssid_label_size = 6;
    if(!ssid_label) {
        ssid_label = strstr(line, "SSID:");
        ssid_label_size = 5;
    }
    const char* ch_label = strstr(line, "Ch:");
    if(!ch_label) ch_label = strstr(line, "CH:");
    if(!ch_label) ch_label = strstr(line, "ch:");

    if(ssid_label && ch_label) {
        const char* rssi_label = strstr(line, "RSSI:");
        if(rssi_label) {
            if(!ns_parse_int(rssi_label + 5, &rssi)) return false;
        } else {
            const char* p = ns_skip_space(line);
            if(*p == '>') p = ns_skip_space(p + 1);
            if(*p == '#') p = ns_skip_space(p + 1);
            if(!ns_parse_int(p, &rssi)) return false;
        }
        if(!ns_parse_int(ch_label + 3, &ch)) return false;
        if(ch < 1 || ch > 14 || rssi < -127 || rssi > 20) return false;
        const char* mac = ns_find_mac(line);
        if(!mac) return false;
        memcpy(out->bssid, mac, 17);
        out->bssid[17] = 0;
        out->rssi = (int16_t)rssi;
        out->channel = (uint8_t)ch;
        const char* start = ssid_label + ssid_label_size;
        ns_copy_ssid(out->ssid, sizeof(out->ssid), start, start + strlen(start));
        return true;
    }

    /* Some releases list results as: [0][CH:3] Octoglass -66
     * This format has no BSSID; report it honestly rather than inventing one.
     */
    const char* p = ns_skip_space(line);
    if(*p != '[') return false;
    const char* end_index = strchr(p, ']');
    if(!end_index || end_index == p + 1 || end_index[1] != '[') return false;
    const char* header = end_index + 2;
    if(strncmp(header, "CH:", 3) && strncmp(header, "Ch:", 3)) return false;
    if(!ns_parse_int(header + 3, &ch) || ch < 1 || ch > 14) return false;
    const char* bracket = strchr(header, ']');
    if(!bracket) return false;
    const char* name_start = ns_skip_space(bracket + 1);
    const char* name_end = name_start + strlen(name_start);
    while(name_end > name_start && isspace((unsigned char)name_end[-1])) name_end--;
    /* Locate the final RSSI token (e.g. -66), not an embedded SSID number. */
    const char* rssi_start = name_end;
    while(rssi_start > name_start && !isspace((unsigned char)rssi_start[-1])) rssi_start--;
    if(rssi_start <= name_start || *rssi_start != '-' || !ns_parse_int(rssi_start, &rssi)) return false;
    if(rssi < -127 || rssi > 20) return false;
    while(rssi_start > name_start && isspace((unsigned char)rssi_start[-1])) rssi_start--;
    out->channel = (uint8_t)ch;
    out->rssi = (int16_t)rssi;
    ns_copy_ssid(out->ssid, sizeof(out->ssid), name_start, rssi_start);
    return true;
}
