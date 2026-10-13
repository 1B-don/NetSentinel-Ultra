#include "../../include/netsentinel/persistence.h"
#include <string.h>
/* Fixed LE schema: magic, generation, last ID, count, 4 settings words,
 * 16 optional history records (6 words each), trailing CRC32. */
static void put(uint8_t* p, uint32_t v) {
    for(unsigned i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (8 * i));
}
static uint32_t get(const uint8_t* p) {
    uint32_t v = 0; for(unsigned i = 0; i < 4; i++) v |= (uint32_t)p[i] << (8 * i); return v;
}
static uint32_t crc(const uint8_t* p, size_t n) {
    uint32_t v = UINT32_MAX;
    for(size_t i = 0; i < n; i++) {
        v ^= p[i];
        for(unsigned j = 0; j < 8; j++) v = (v >> 1) ^ (0xedb88320u & (0u - (v & 1u)));
    }
    return ~v;
}
void ns_saved_defaults(NsSavedState* s) { memset(s, 0, sizeof(*s)); s->settings = ns_settings_default(); }
static bool valid(const NsSavedState* s) {
    char check[100];
    if(s->history_count > NS_HISTORY_LIMIT || ns_settings_format(&s->settings, check, sizeof(check)) != NS_OK) return false;
    uint32_t prior = 0;
    for(size_t i = 0; i < s->history_count; i++) {
        const NsHistory* h = &s->history[i];
        if(!h->id || h->id <= prior || h->id > s->last_id || h->radio < NS_RADIO_ESP ||
           h->radio > NS_RADIO_CC || (unsigned)h->result > NS_FULL) return false;
        prior = h->id;
    }
    return true;
}
static size_t encode(const NsSavedState* s, uint32_t generation, uint8_t* b) {
    memcpy(b, "NSP1", 4); put(b+4, generation); put(b+8, s->last_id); put(b+12, (uint32_t)s->history_count);
    put(b+16, s->settings.baud); put(b+20, s->settings.timeout_ms);
    put(b+24, s->settings.dwell_us); put(b+28, s->settings.cc_frequency_hz);
    size_t pos = 32;
    for(size_t i = 0; i < s->history_count; i++, pos += 24) {
        const NsHistory* h = &s->history[i];
        put(b+pos, h->id); put(b+pos+4, h->started_ms); put(b+pos+8, h->ended_ms);
        put(b+pos+12, h->observations); put(b+pos+16, h->radio); put(b+pos+20, h->result);
    }
    put(b+pos, crc(b, pos)); return pos+4;
}
static bool decode(const uint8_t* b, size_t n, NsSavedState* s, uint32_t* generation) {
    if(n < 36 || memcmp(b, "NSP1", 4) || !get(b+4) || get(b+12) > NS_HISTORY_LIMIT ||
       n != 36 + (size_t)get(b+12) * 24 || get(b+n-4) != crc(b, n-4)) return false;
    ns_saved_defaults(s); *generation = get(b+4); s->last_id = get(b+8); s->history_count = get(b+12);
    s->settings = (NsSettings){get(b+16), get(b+20), get(b+24), get(b+28)};
    for(size_t i = 0; i < s->history_count; i++) {
        size_t pos = 32+i*24;
        /* Validate full stored words before conversion to possibly short enums.
         * Otherwise values such as 256 could narrow into a valid result. */
        uint32_t radio = get(b+pos+16), result = get(b+pos+20);
        if(radio < NS_RADIO_ESP || radio > NS_RADIO_CC || result > NS_FULL) return false;
        s->history[i] = (NsHistory){get(b+pos), get(b+pos+4), get(b+pos+8), get(b+pos+12),
            (NsRadio)radio, (NsResult)result};
    }
    return valid(s);
}
NsResult ns_persistence_load(NsPersistence* p, const NsStore* io, NsSavedState* out) {
    if(!p || !io || !io->read || !io->write || !out) return NS_INVALID;
    memset(p, 0, sizeof(*p)); p->io = io;
    NsSavedState candidate, best; uint32_t gen = 0;
    bool found = false, absent = true;
    NsResult failure = NS_INVALID;
    for(unsigned slot = 0; slot < 2; slot++) {
        uint8_t data[NS_STATE_CAPACITY]; size_t n = 0;
        NsResult r = io->read(io->ctx, slot, data, sizeof(data), &n);
        if(r != NS_ABSENT) absent = false;
        if(r != NS_OK && r != NS_ABSENT) failure = r;
        if(r == NS_OK && n <= sizeof(data) && decode(data, n, &candidate, &gen) && (!found || gen > p->generation)) {
            best = candidate; p->generation = gen; p->slot = slot; found = true;
        }
    }
    if(found) { *out = best; p->loaded = true; return NS_OK; }
    if(absent) { ns_saved_defaults(out); p->loaded = true; return NS_ABSENT; }
    return failure; /* do not overwrite corrupted or unreadable slots */
}
NsResult ns_persistence_save(NsPersistence* p, const NsSavedState* s) {
    if(!p || !p->loaded || !p->io || !s || !valid(s)) return NS_INVALID;
    if(p->generation == UINT32_MAX) return NS_FULL;
    unsigned slot = p->generation ? p->slot ^ 1u : 0;
    uint8_t data[NS_STATE_CAPACITY], verify[NS_STATE_CAPACITY];
    size_t n = encode(s, p->generation+1, data), read_n = 0;
    NsResult r = p->io->write(p->io->ctx, slot, data, n);
    if(r != NS_OK) return r;
    r = p->io->read(p->io->ctx, slot, verify, sizeof(verify), &read_n);
    if(r != NS_OK) return r;
    if(read_n != n || memcmp(data, verify, n)) return NS_IO;
    p->slot = slot; p->generation++; return NS_OK;
}
