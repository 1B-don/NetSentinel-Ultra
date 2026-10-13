#ifndef NS_RADIOS_H
#define NS_RADIOS_H
#include "platform.h"
#define NS_NRF_CHANNELS 126
typedef struct {
    const NsPlatform* io;
    bool owned;
    bool ready;
    uint32_t samples[NS_NRF_CHANNELS];
    uint32_t hits[NS_NRF_CHANNELS];
} NsNrf;
NsResult ns_nrf_open(NsNrf*, const NsPlatform*, bool selector_confirmed);
NsResult ns_nrf_sample(NsNrf*, uint8_t channel, uint32_t dwell_us);
NsResult ns_nrf_close(NsNrf*);
unsigned ns_nrf_percent(const NsNrf*, uint8_t);
#define NS_CC_TRACE 64
typedef struct {
    const NsPlatform* io;
    bool owned;
    bool ready;
    bool diagnostic_valid;
    NsCcDiagnostics diagnostic;
    int16_t trace[NS_CC_TRACE];
    size_t trace_count, trace_next;
    uint32_t frequency;
    uint32_t samples;
    int16_t last, minimum, maximum;
} NsCc;
bool ns_cc_frequency_valid(uint32_t);
NsResult ns_cc_open(NsCc*, const NsPlatform*, bool selector_confirmed);
NsResult ns_cc_tune(NsCc*, uint32_t);
NsResult ns_cc_sample(NsCc*);
NsResult ns_cc_close(NsCc*);
NsResult ns_cc_diagnose(NsCc*);
uint8_t ns_cc_graph_height(int16_t rssi_dbm, uint8_t height);
/* Fraction of recent samples >= a caller-selected threshold, not packet count. */
unsigned ns_cc_above_percent(const NsCc*, int16_t threshold_dbm);
int16_t ns_cc_recent_mean(const NsCc*);
#endif
