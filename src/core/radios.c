#include "../../include/netsentinel/radios.h"
#include <string.h>
/* Independently authored from documented nRF24L01+ register interface.
 * CONFIG is always primary RX; auto-ACK/retries are disabled before CE high.
 * No payload write, TX mode, carrier mode or TX command is exposed. */
static NsResult nrf_reg(NsNrf* n, bool write, uint8_t reg, uint8_t* value) {
    uint8_t tx[2] = {(uint8_t)(reg | (write ? 0x20 : 0)), write ? *value : 0xff};
    uint8_t rx[2] = {0};
    NsResult r = n->io->spi(n->io->ctx, tx, rx, 2, NS_NRF_SPI_TIMEOUT_MS);
    if(r == NS_OK && !write) *value = rx[1];
    return r;
}
static NsResult nrf_set(NsNrf* n, uint8_t reg, uint8_t value) {
    NsResult r = nrf_reg(n, true, reg, &value);
    if(r != NS_OK) return r;
    uint8_t readback = 0;
    r = nrf_reg(n, false, reg, &readback);
    return r != NS_OK ? r : (readback == value ? NS_OK : NS_ABSENT);
}
NsResult ns_nrf_close(NsNrf* n) {
    if(!n) return NS_INVALID;
    if(!n->owned) return NS_OK;
    n->io->ce(n->io->ctx, false);
    uint8_t config = 0x01; /* power down, retain RX mode */
    NsResult r = nrf_reg(n, true, 0, &config);
    n->io->release(n->io->ctx, NS_RADIO_NRF);
    n->owned = n->ready = false;
    return r;
}
NsResult ns_nrf_open(NsNrf* n, const NsPlatform* io, bool confirmed) {
    if(!n || !io || !io->acquire || !io->release || !io->spi || !io->ce || !io->delay_us) return NS_INVALID;
    if(n->owned) return NS_BUSY;
    if(!confirmed) return NS_INVALID;
    memset(n, 0, sizeof(*n)); n->io = io;
    NsResult r = io->acquire(io->ctx, NS_RADIO_NRF);
    if(r != NS_OK) return r;
    n->owned = true; io->ce(io->ctx, false);
    const uint8_t regs[] = {0, 1, 4, 2, 6, 5, 0};
    const uint8_t vals[] = {1, 0, 0, 0, 0, 42, 3};
    for(size_t i = 0; i < sizeof(regs); i++) {
        r = nrf_set(n, regs[i], vals[i]);
        if(r != NS_OK) { ns_nrf_close(n); return r; }
    }
    io->delay_us(io->ctx, 5000);
    n->ready = true; return NS_OK;
}
NsResult ns_nrf_sample(NsNrf* n, uint8_t channel, uint32_t dwell) {
    if(!n || !n->ready || channel >= NS_NRF_CHANNELS || dwell < 170 || dwell > 100000) return NS_INVALID;
    if(n->samples[channel] == UINT32_MAX) { ns_nrf_close(n); return NS_FULL; }
    n->io->ce(n->io->ctx, false);
    NsResult r = nrf_set(n, 5, channel);
    uint8_t rpd = 0;
    if(r == NS_OK) {
        n->io->ce(n->io->ctx, true);
        n->io->delay_us(n->io->ctx, dwell);
        n->io->ce(n->io->ctx, false);
        r = nrf_reg(n, false, 9, &rpd);
    }
    if(r != NS_OK) { ns_nrf_close(n); return r; }
    n->samples[channel]++; n->hits[channel] += rpd & 1;
    return NS_OK;
}
unsigned ns_nrf_percent(const NsNrf* n, uint8_t ch) {
    if(!n || ch >= NS_NRF_CHANNELS || !n->samples[ch]) return 0;
    return (unsigned)((uint64_t)n->hits[ch] * 100 / n->samples[ch]);
}
bool ns_cc_frequency_valid(uint32_t f) {
    return (f >= 300000000 && f <= 348000000) ||
           (f >= 387000000 && f <= 464000000) ||
           (f >= 779000000 && f <= 928000000);
}
NsResult ns_cc_close(NsCc* c) {
    if(!c) return NS_INVALID;
    if(!c->owned) return NS_OK;
    NsResult r = c->io->cc_idle(c->io->ctx, NS_CC_IO_TIMEOUT_MS);
    c->io->release(c->io->ctx, NS_RADIO_CC);
    c->owned = c->ready = false;
    return r;
}
NsResult ns_cc_open(NsCc* c, const NsPlatform* io, bool confirmed) {
    if(!c || !io || !io->acquire || !io->release || !io->cc_probe || !io->cc_receive || !io->cc_rssi || !io->cc_idle) return NS_INVALID;
    if(c->owned) return NS_BUSY;
    if(!confirmed) return NS_INVALID;
    memset(c, 0, sizeof(*c)); c->io = io;
    NsResult r = io->acquire(io->ctx, NS_RADIO_CC);
    if(r != NS_OK) return r;
    c->owned = true;
    r = io->cc_probe(io->ctx, NS_CC_PROBE_TIMEOUT_MS);
    if(r != NS_OK) { ns_cc_close(c); return r; }
    c->ready = true;
    if(io->cc_diagnose) {
        r = ns_cc_diagnose(c);
        if(r != NS_OK) { ns_cc_close(c); return r; }
    }
    return NS_OK;
}
NsResult ns_cc_diagnose(NsCc* c) {
    if(!c || !c->ready || !c->owned) return NS_INVALID;
    if(!c->io->cc_diagnose) return NS_UNSUPPORTED;
    NsCcDiagnostics d = {0};
    NsResult r = c->io->cc_diagnose(c->io->ctx, &d, NS_CC_DIAG_TIMEOUT_MS);
    c->diagnostic_valid = false;
    if(r != NS_OK) return r;
    if(d.part != 0 || d.version == 0 || d.version == 0xff) return NS_ABSENT;
    if(d.state > 0x1f) return NS_IO;
    c->diagnostic = d; c->diagnostic_valid = true; return NS_OK;
}
NsResult ns_cc_tune(NsCc* c, uint32_t f) {
    if(!c || !c->ready || !ns_cc_frequency_valid(f)) return NS_INVALID;
    NsResult r = c->io->cc_idle(c->io->ctx, NS_CC_IO_TIMEOUT_MS);
    if(r == NS_OK) r = c->io->cc_receive(c->io->ctx, f, NS_CC_TUNE_TIMEOUT_MS);
    if(r != NS_OK) { ns_cc_close(c); return r; }
    if(r == NS_OK && c->io->cc_diagnose) {
        r = ns_cc_diagnose(c);
        if(r == NS_OK && c->diagnostic.state != 0x0d) r = NS_IO;
        if(r != NS_OK) { ns_cc_close(c); return r; }
    }
    c->frequency = f; c->samples = 0; c->last = c->minimum = c->maximum = 0;
    c->trace_count = c->trace_next = 0; return NS_OK;
}
NsResult ns_cc_sample(NsCc* c) {
    if(!c || !c->ready || !c->frequency) return NS_INVALID;
    if(c->samples == UINT32_MAX) { ns_cc_close(c); return NS_FULL; }
    int16_t value = 0;
    NsResult r = NS_OK;
    if(c->io->cc_diagnose) {
        r = ns_cc_diagnose(c);
        if(r == NS_OK && c->diagnostic.state != 0x0d) r = NS_IO;
    }
    if(r == NS_OK) r = c->io->cc_rssi(c->io->ctx, &value, NS_CC_IO_TIMEOUT_MS);
    if(r != NS_OK) { ns_cc_close(c); return r; }
    if(value < -150 || value > 20) { ns_cc_close(c); return NS_IO; }
    c->last = value;
    if(!c->samples || value < c->minimum) c->minimum = value;
    if(!c->samples || value > c->maximum) c->maximum = value;
    c->trace[c->trace_next] = value;
    c->trace_next = (c->trace_next+1) % NS_CC_TRACE;
    if(c->trace_count < NS_CC_TRACE) c->trace_count++;
    c->samples++; return NS_OK;
}
uint8_t ns_cc_graph_height(int16_t rssi, uint8_t height) {
    if(rssi <= -120) return 0;
    if(rssi >= -20) return height;
    return (uint8_t)((uint32_t)(rssi+120)*height/100);
}
unsigned ns_cc_above_percent(const NsCc* c, int16_t threshold) {
    if(!c || !c->trace_count) return 0;
    unsigned hits = 0;
    for(size_t i = 0; i < c->trace_count; i++) if(c->trace[i] >= threshold) hits++;
    return (unsigned)(hits*100/c->trace_count);
}
int16_t ns_cc_recent_mean(const NsCc* c) {
    if(!c || !c->trace_count) return 0;
    int32_t sum = 0;
    for(size_t i = 0; i < c->trace_count; i++) sum += c->trace[i];
    return (int16_t)(sum/(int32_t)c->trace_count);
}
