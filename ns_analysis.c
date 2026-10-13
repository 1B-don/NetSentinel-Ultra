#include "ns_analysis.h"
#include <stdio.h>
#include <string.h>

void ns_channel_counts(const NsAccessPoint* aps, size_t count, uint8_t channels[14]) {
    memset(channels, 0, 14);
    for(size_t i = 0; i < count; ++i) {
        if(aps[i].channel >= 1 && aps[i].channel <= 14 && channels[aps[i].channel - 1] < UINT8_MAX) {
            channels[aps[i].channel - 1]++;
        }
    }
}

static bool ns_matches(const NsAccessPoint* a, const NsAccessPoint* b) {
    if(a->bssid[0] && b->bssid[0]) return strcmp(a->bssid, b->bssid) == 0;
    /* Fall back to SSID+channel if firmware did not report a MAC. This
       intentionally cannot detect channel changes without a BSSID. */
    return strcmp(a->ssid, b->ssid) == 0 && a->channel == b->channel;
}

NsDiff ns_diff(const NsAccessPoint* before, size_t n_before,
               const NsAccessPoint* after, size_t n_after) {
    NsDiff result = {0};
    for(size_t i = 0; i < n_after; i++) {
        bool found = false;
        for(size_t j = 0; j < n_before; j++) {
            if(ns_matches(&after[i], &before[j])) {
                found = true;
                if(after[i].channel != before[j].channel) result.channel_changes++;
                break;
            }
        }
        if(!found) result.new_aps++;
    }
    for(size_t i = 0; i < n_before; i++) {
        bool found = false;
        for(size_t j = 0; j < n_after; j++) {
            if(ns_matches(&before[i], &after[j])) { found = true; break; }
        }
        if(!found) result.missing_aps++;
    }
    return result;
}

bool ns_csv_ap(const NsAccessPoint* ap, char* out, size_t cap) {
    if(!ap || !out || cap == 0) return false;
    size_t i = 0;
#define ADD_CH(c) do { if(i + 1 >= cap) return false; out[i++] = (c); } while(0)
    ADD_CH('"');
    for(const char* p = ap->ssid; *p; ++p) {
        if(*p == '"') ADD_CH('"');
        ADD_CH(*p);
    }
    ADD_CH('"'); ADD_CH(','); ADD_CH('"');
    for(const char* p = ap->bssid; *p; ++p) { if(*p == '"') ADD_CH('"'); ADD_CH(*p); }
    ADD_CH('"');
    int n = snprintf(out + i, cap - i, ",%u,%d\n", ap->channel, ap->rssi);
    if(n < 0 || (size_t)n >= cap - i) return false;
#undef ADD_CH
    return true;
}
