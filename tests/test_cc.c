#include "netsentinel/cc_spi.h"
#include "netsentinel/session.h"
#include "mock_platform.h"
#include <stdio.h>
typedef struct {
    uint8_t part, version, state, raw, chip_status;
    unsigned calls, fail_at;
    bool unstable, identity_flip;
} BusMock;
static NsResult transfer(void* ctx, const uint8_t* tx, uint8_t* rx, size_t count) {
    BusMock* b = ctx;
    assert(count == 2 && !tx[1]);
    /* Reject every strobe, write, FIFO access and undocumented status read. */
    assert(tx[0] == 0xf0 || tx[0] == 0xf1 || tx[0] == 0xf4 || tx[0] == 0xf5);
    b->calls++; if(b->calls == b->fail_at) return NS_TIMEOUT;
    rx[0] = b->chip_status;
    switch(tx[0]) {
    case 0xf0: rx[1] = b->part; break;
    case 0xf1: rx[1] = b->version + (b->identity_flip && b->calls == 4 ? 1 : 0); break;
    case 0xf4: rx[1] = b->raw; break;
    default: rx[1] = b->state + (b->unstable ? b->calls%2 : 0); break;
    }
    return NS_OK;
}
typedef struct { uint8_t config,status; unsigned calls,fail_at; bool unstable; } PolicyMock;
static NsResult policy_transfer(void* ctx,const uint8_t* tx,uint8_t* rx,size_t n) {
    PolicyMock* p=ctx; assert(n==2 && tx[0]==0x97 && !tx[1]);
    p->calls++; if(p->calls==p->fail_at) return NS_IO;
    rx[0]=p->status;rx[1]=p->config^(p->unstable && p->calls==2 ? 1 : 0);return NS_OK;
}
static void policy_cases(void) {
    PolicyMock p={0};NsCcBus bus={&p,policy_transfer};uint8_t out;
    for(unsigned v=0;v<256;v++) {
        p.config=(uint8_t)v;p.calls=0;out=0xa5;unsigned mode=(v>>2)&3;
        NsResult r=ns_cc_spi_check_rx_policy(&bus,&out);
        assert(r==(mode==0||mode==3 ? NS_OK : NS_UNSUPPORTED) && p.calls==2);
        assert(out==(r==NS_OK ? v : 0xa5));
    }
    p.config=0;p.fail_at=1;p.calls=0;out=0xa5;assert(ns_cc_spi_check_rx_policy(&bus,&out)==NS_IO && out==0xa5);
    p.fail_at=2;p.calls=0;assert(ns_cc_spi_check_rx_policy(&bus,&out)==NS_IO && out==0xa5);
    p.fail_at=0;p.calls=0;p.status=0x80;assert(ns_cc_spi_check_rx_policy(&bus,&out)==NS_TIMEOUT && out==0xa5);
    p.status=0;p.calls=0;p.unstable=true;assert(ns_cc_spi_check_rx_policy(&bus,&out)==NS_IO && out==0xa5);
    assert(ns_cc_spi_check_rx_policy(NULL,&out)==NS_INVALID);
}
static void bus_cases(void) {
    BusMock b = {.version=0x14, .state=0x0d, .raw=0xd8};
    NsCcBus bus = {&b, transfer}; NsCcDiagnostics d = {0};
    assert(ns_cc_spi_diagnose(&bus, &d) == NS_OK);
    assert(d.part == 0 && d.version == 0x14 && d.state == 13 && d.rssi_raw == 0xd8 && d.transactions == 7);
    assert(ns_cc_rssi_half_dbm(d.rssi_raw, 74) == -188); /* -94 dBm */
    NsCcDiagnostics sentinel = d;
    for(unsigned i = 1; i <= 7; i++) {
        b.calls = 0; b.fail_at = i;
        assert(ns_cc_spi_diagnose(&bus, &d) == NS_TIMEOUT && !memcmp(&d, &sentinel, sizeof(d)));
    }
    b.fail_at = 0; b.calls = 0; b.unstable = true;
    assert(ns_cc_spi_diagnose(&bus, &d) == NS_TIMEOUT && b.calls == 10);
    b.unstable = false; b.calls = 0; b.identity_flip = true;
    assert(ns_cc_spi_diagnose(&bus, &d) == NS_IO);
    b.identity_flip = false; b.calls = 0; b.version = 0;
    assert(ns_cc_spi_diagnose(&bus, &d) == NS_ABSENT);
    b.version = 0xff; assert(ns_cc_spi_diagnose(&bus, &d) == NS_ABSENT);
    b.version = 0x14; b.part = 0xff; assert(ns_cc_spi_diagnose(&bus, &d) == NS_ABSENT);
    b.part = 0; b.chip_status = 0x80; assert(ns_cc_spi_diagnose(&bus, &d) == NS_TIMEOUT);
    b.chip_status = 0; b.version = 0x25; /* future revision retained, not authenticity proof */
    assert(ns_cc_spi_diagnose(&bus, &d) == NS_OK && d.version == 0x25);
    for(unsigned raw = 0; raw <= 255; raw++) {
        int signed_raw = raw < 128 ? (int)raw : (int)raw-256;
        assert(ns_cc_rssi_half_dbm((uint8_t)raw, 74) == signed_raw-148);
    }
    assert(ns_cc_spi_diagnose(NULL, &d) == NS_INVALID);
}
typedef struct { Mock io; BusMock bus; } Combined;
static NsResult diagnose(void* ctx, NsCcDiagnostics* out) {
    Combined* c = ctx; assert(c->io.owner == NS_RADIO_CC);
    NsCcBus bus = {&c->bus, transfer}; return ns_cc_spi_diagnose(&bus, out);
}
static void driver_cases(void) {
    Combined m = {.io={.rssi=-100}, .bus={.version=0x14, .state=0x0d, .raw=0xd8}};
    NsPlatform io = platform(&m.io); io.cc_diagnose = diagnose;
    NsCc c = {0};
    assert(ns_cc_open(&c, &io, true) == NS_OK && c.diagnostic_valid);
    assert(ns_cc_tune(&c, 433920000) == NS_OK);
    assert(ns_cc_tune(&c, 350000000) == NS_INVALID && c.ready && m.io.frequency == 433920000);
    for(unsigned i = 0; i < 96; i++) {
        m.io.rssi = i < 32 ? -100 : (i%2 ? -50 : -90);
        assert(ns_cc_sample(&c) == NS_OK);
    }
    assert(c.trace_count == 64 && c.trace_next == 32 && c.samples == 96);
    assert(ns_cc_above_percent(&c, -80) == 50 && ns_cc_recent_mean(&c) == -70);
    assert(c.minimum == -100 && c.maximum == -50);
    assert(ns_cc_graph_height(-150, 16) == 0 && ns_cc_graph_height(-70, 16) == 8 && ns_cc_graph_height(20, 16) == 16);
    assert(ns_cc_tune(&c, 868000000) == NS_OK && !c.samples && !c.trace_count);
    m.bus.state = 0x13; /* unexpected TX state: no RSSI accepted */
    assert(ns_cc_sample(&c) == NS_IO && !m.io.owner && !m.io.frequency);
    m.bus.state = 0x0d; assert(ns_cc_open(&c, &io, true) == NS_OK);
    m.bus.state = 1; assert(ns_cc_tune(&c, 915000000) == NS_IO && !m.io.owner);
    m.bus.state = 0x0d; m.bus.part = 1;
    assert(ns_cc_open(&c, &io, true) == NS_ABSENT && !m.io.owner);
    assert(m.io.acquired == m.io.released);
}
static void session_cases(void) {
    Combined m = {.io={.rssi=-95}, .bus={.version=0x14, .state=13}};
    NsPlatform io = platform(&m.io); io.cc_diagnose = diagnose; NsSession s;
    ns_session_init(&s, &io, NULL);
    assert(ns_session_start(&s, NS_RADIO_CC, false, true, UINT32_MAX-50) == NS_OK);
    assert(ns_cc_tune(&s.cc, 433920000) == NS_OK);
    assert(ns_session_log_open(&s, "/ext/apps_data/netsentinel/session_0000000001.csv") == NS_OK);
    assert(strstr(m.io.file, "cc_config,activity_threshold,433920000,-80,dBm"));
    assert(strstr(m.io.file, "cc_diag,version,433920000,20,byte"));
    assert(ns_session_tick(&s, UINT32_MAX-50) == NS_OK && s.observations == 1);
    assert(ns_session_tick(&s, 48) == NS_OK && s.observations == 1);
    m.io.rssi = -45; assert(ns_session_tick(&s, 49) == NS_OK && s.observations == 2);
    assert(strstr(m.io.file, "cc_activity,above_-80,433920000,50,percent"));
    assert(ns_session_cc_tune(&s, 350000000, 50) == NS_INVALID && s.active == NS_RADIO_CC);
    assert(ns_session_cc_tune(&s, 915000000, 50) == NS_OK && !s.cc.trace_count);
    assert(ns_session_tick(&s, 50) == NS_OK && s.observations == 3);
    assert(strstr(m.io.file, "cc_tune,,915000000,,Hz"));
    m.io.fail_write = true;
    assert(ns_session_tick(&s, 150) == NS_IO && !m.io.owner && s.history[0].result == NS_IO);
    assert(m.io.acquired == m.io.released && m.io.closes == 1);
    m.io.fail_write = false;
    assert(ns_session_start(&s, NS_RADIO_CC, false, true, 200) == NS_OK);
    assert(ns_cc_tune(&s.cc, 433920000) == NS_OK);
    m.bus.unstable = true; assert(ns_session_cc_diagnose(&s, 201) == NS_TIMEOUT && !m.io.owner);
}
int main(void) { policy_cases(); bus_cases(); driver_cases(); session_cases(); puts("PASS: SIMULATED CC1101 status SPI, bounded faults, RX-state checks, RSSI trace/retune/cadence and CSV cleanup"); }
