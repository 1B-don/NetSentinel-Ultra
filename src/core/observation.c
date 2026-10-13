#include "../../include/netsentinel/observation.h"
NsObservationFilter ns_observation_filter_default(void) {
    return (NsObservationFilter){0, -127, 0};
}
NsResult ns_observation_select(const NsAccessPoint* aps, const uint32_t* seen, size_t count,
    const NsObservationFilter* f, uint32_t now, uint8_t* indices, size_t capacity, size_t* selected) {
    if(!f || !indices || !selected || count > NS_AP_LIMIT || (count && (!aps || !seen)) ||
       f->channel > 14 || f->minimum_rssi < -127 || f->minimum_rssi > 20 ||
       f->maximum_age_ms > 600000) return NS_INVALID;
    uint8_t matched[NS_AP_LIMIT]; size_t n = 0;
    for(size_t i = 0; i < count; i++) {
        if(aps[i].channel < 1 || aps[i].channel > 14 || aps[i].rssi < -127 || aps[i].rssi > 20) continue;
        if(f->channel && aps[i].channel != f->channel) continue;
        if(aps[i].rssi < f->minimum_rssi) continue;
        if(f->maximum_age_ms && (uint32_t)(now-seen[i]) > f->maximum_age_ms) continue;
        matched[n++] = (uint8_t)i;
    }
    if(n > capacity) return NS_FULL;
    for(size_t i = 0; i < n; i++) indices[i] = matched[i];
    *selected = n; return NS_OK;
}
const NsHistory* ns_history_select(const NsSavedState* s, const NsHistoryFilter* f, size_t ordinal) {
    if(!s || !f || s->history_count > NS_HISTORY_LIMIT || (unsigned)f->radio > NS_RADIO_CC) return NULL;
    for(size_t i = s->history_count; i; i--) {
        const NsHistory* h = &s->history[i-1];
        if(f->radio && h->radio != f->radio) continue;
        if(f->failures_only && h->result == NS_OK) continue;
        if(!ordinal--) return h;
    }
    return NULL;
}
NsResult ns_activity_summary(const NsNrf* n, NsActivitySummary* out) {
    if(!n || !out) return NS_INVALID;
    NsActivitySummary s = {0};
    for(unsigned ch = 0; ch < NS_NRF_CHANNELS; ch++) {
        if(n->hits[ch] > n->samples[ch]) return NS_INVALID;
        if(!n->samples[ch]) continue;
        unsigned percent = ns_nrf_percent(n, (uint8_t)ch);
        if(!s.measured_channels || percent > s.peak_percent) { s.peak_channel = (uint8_t)ch; s.peak_percent = percent; }
        if(!s.measured_channels || percent < s.quiet_percent) { s.quiet_channel = (uint8_t)ch; s.quiet_percent = percent; }
        s.measured_channels++; s.hits += n->hits[ch]; s.samples += n->samples[ch];
    }
    *out = s; return NS_OK;
}
