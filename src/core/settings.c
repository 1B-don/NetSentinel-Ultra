#include "netsentinel/settings.h"
#include "netsentinel/radios.h"
#include <stdio.h>
NsSettings ns_settings_default(void) { return (NsSettings){115200, 5000, 200, 433920000}; }
static bool valid(const NsSettings* s) {
    return s && (s->baud == 9600 || s->baud == 57600 || s->baud == 115200) &&
        s->timeout_ms >= 1000 && s->timeout_ms <= 60000 && s->dwell_us >= 170 &&
        s->dwell_us <= 100000 && ns_cc_frequency_valid(s->cc_frequency_hz);
}
NsResult ns_settings_format(const NsSettings* s, char* out, size_t cap) {
    if(!valid(s) || !out || !cap) return NS_INVALID;
    int n = snprintf(out, cap, "NS1 %lu %lu %lu %lu\n", (unsigned long)s->baud,
        (unsigned long)s->timeout_ms, (unsigned long)s->dwell_us, (unsigned long)s->cc_frequency_hz);
    return n < 0 || (size_t)n >= cap ? NS_FULL : NS_OK;
}
/* Strict fixed schema, bounded decimal values; reject unknown/trailing data.
 * Parse transactionally so corrupt storage never partly updates settings. */
NsResult ns_settings_parse(const char* text, NsSettings* out) {
    if(!text || !out || text[0] != 'N' || text[1] != 'S' || text[2] != '1' || text[3] != ' ') return NS_INVALID;
    uint32_t fields[4] = {0}; const char* p = text + 4;
    for(unsigned i = 0; i < 4; i++) {
        if(*p < '0' || *p > '9') return NS_INVALID;
        while(*p >= '0' && *p <= '9') {
            unsigned d = (unsigned)(*p++ - '0');
            if(fields[i] > (UINT32_MAX - d) / 10) return NS_INVALID;
            fields[i] = fields[i] * 10 + d;
        }
        if(i < 3 && *p++ != ' ') return NS_INVALID;
    }
    if(*p == '\n') p++;
    if(*p) return NS_INVALID;
    NsSettings s = {fields[0], fields[1], fields[2], fields[3]};
    if(!valid(&s)) return NS_INVALID;
    *out = s; return NS_OK;
}
uint8_t ns_graph_height(uint32_t hits, uint32_t samples, uint8_t height) {
    if(!samples) return 0;
    if(hits > samples) hits = samples;
    return (uint8_t)((uint64_t)hits * height / samples);
}
