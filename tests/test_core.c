#include "netsentinel/session.h"
#include "netsentinel/settings.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "mock_platform.h"
static void test_nrf(void) {
    Mock m = {0}; NsPlatform io = platform(&m); NsNrf n = {0};
    assert(ns_nrf_open(&n, &io, false) == NS_INVALID && !m.acquired);
    m.absent = true;
    assert(ns_nrf_open(&n, &io, true) == NS_ABSENT && m.released == 1 && !m.ce_high);
    m.absent = false;
    assert(ns_nrf_open(&n, &io, true) == NS_OK);
    assert(ns_nrf_open(&n, &io, true) == NS_BUSY);
    assert(ns_nrf_sample(&n, 126, 200) == NS_INVALID);
    assert(ns_nrf_sample(&n, 0, 169) == NS_INVALID);
    m.regs[9] = 1;
    for(unsigned i = 0; i < NS_NRF_CHANNELS; i++) assert(ns_nrf_sample(&n, i, 200) == NS_OK);
    assert(ns_nrf_percent(&n, 125) == 100);
    m.regs[9] = 0; assert(ns_nrf_sample(&n, 125, 200) == NS_OK);
    assert(ns_nrf_percent(&n, 125) == 50);
    m.fail_spi = true;
    assert(ns_nrf_sample(&n, 1, 200) == NS_IO && !m.owner && !n.ready);
    unsigned released = m.released; ns_nrf_close(&n); assert(m.released == released);
}
static void test_spi_fault_matrix(void) {
    for(unsigned at = 1; at <= 14; at++) {
        Mock m = {.fail_at = at}; NsPlatform io = platform(&m); NsNrf n = {0};
        assert(ns_nrf_open(&n, &io, true) == NS_IO);
        assert(!m.owner && !m.ce_high && m.acquired == m.released);
    }
    for(unsigned offset = 1; offset <= 3; offset++) {
        Mock m = {0}; NsPlatform io = platform(&m); NsNrf n = {0};
        assert(ns_nrf_open(&n, &io, true) == NS_OK);
        m.fail_at = m.transfers + offset;
        assert(ns_nrf_sample(&n, 5, 200) == NS_IO);
        assert(!m.owner && !m.ce && !n.samples[5]);
    }
}
static void test_cc(void) {
    Mock m = {.rssi = -74}; NsPlatform io = platform(&m); NsCc c = {0};
    assert(!ns_cc_frequency_valid(350000000)); assert(ns_cc_frequency_valid(433920000));
    assert(ns_cc_open(&c, &io, false) == NS_INVALID);
    assert(ns_cc_open(&c, &io, true) == NS_OK);
    assert(ns_cc_sample(&c) == NS_INVALID);
    assert(ns_cc_tune(&c, 433920000) == NS_OK);
    assert(ns_cc_sample(&c) == NS_OK && c.minimum == -74);
    m.rssi = -50; assert(ns_cc_sample(&c) == NS_OK && c.maximum == -50 && c.samples == 2);
    m.rssi = -151; assert(ns_cc_sample(&c) == NS_IO && !m.owner && !m.frequency);
    m.absent = true; assert(ns_cc_open(&c, &io, true) == NS_ABSENT && !m.owner);
}
static void test_observation_counter_limits(void) {
    Mock m={0};NsPlatform io=platform(&m);NsSession s;ns_session_init(&s,&io,NULL);
    assert(ns_session_start(&s,NS_RADIO_NRF,false,true,0)==NS_OK);s.observations=UINT32_MAX;
    assert(ns_session_tick(&s,1)==NS_FULL && !m.owner && !m.ce && s.observations==UINT32_MAX);
    assert(s.history[0].result==NS_FULL);
    assert(ns_session_start(&s,NS_RADIO_CC,false,true,2)==NS_OK);
    assert(ns_cc_tune(&s.cc,433920000)==NS_OK);s.observations=UINT32_MAX;
    assert(ns_session_tick(&s,3)==NS_FULL && !m.owner && !s.active && s.observations==UINT32_MAX);
}
static void test_session(void) {
    Mock m = {0}; NsPlatform io = platform(&m); NsSession s;
    ns_session_init(&s, &io, NULL);
    assert(ns_session_start(&s, NS_RADIO_ESP, false, false, 0) == NS_UNSUPPORTED);
    assert(!m.uart_calls && !m.acquired);
    /* Synthetic mock protocol, NOT commands for an installed ESP32. */
    const NsEspProfile p = {"mock-only", "MOCK_WIFI_RX\n", "MOCK_BLE_RX\n", "MOCK_STOP\n", true};
    ns_session_init(&s, &io, &p);
    assert(ns_session_start(&s, NS_RADIO_ESP, false, false, UINT32_MAX - 100) == NS_OK);
    assert(ns_session_start(&s, NS_RADIO_NRF, false, true, 0) == NS_BUSY);
    assert(ns_session_log_open(&s, "/ext/apps_data/netsentinel/../bad") == NS_INVALID);
    assert(ns_session_log_open(&s, "/ext/apps_data/netsentinel/session_001.csv") == NS_OK);
    const char* line = "RSSI: -55 Ch: 6 BSSID: 00:11:22:33:44:55 ESSID: a,\"b\"\r\n";
    for(size_t i = 0; i < strlen(line); i++) assert(ns_session_feed(&s, (const uint8_t*)line+i, 1, 50) == NS_OK);
    assert(s.ap_count == 1 && s.aps[0].rssi == -55);
    assert(strstr(m.file, "uart_redacted") && !strstr(m.file, "a,\"\"b"));
    line = "RSSI: -60 Ch: 11 BSSID: 00:11:22:33:44:55 ESSID: renamed\n";
    assert(ns_session_feed(&s, (const uint8_t*)line, strlen(line), 100) == NS_OK);
    assert(s.ap_count == 1 && s.aps[0].channel == 11 && !strcmp(s.aps[0].ssid, "renamed"));
    uint8_t junk[300]; memset(junk, 'x', sizeof(junk));
    assert(ns_session_feed(&s, junk, sizeof(junk), 200) == NS_OK);
    assert(ns_session_feed(&s, (const uint8_t*)"\n", 1, 200) == NS_OK && s.dropped_lines == 1);
    assert(ns_session_tick(&s, 5099) == NS_OK);
    assert(ns_session_tick(&s, 5100) == NS_TIMEOUT && !m.owner && !s.logging);
    assert(s.history_count == 1 && s.history[0].result == NS_TIMEOUT && m.closes == 1);
    assert(ns_session_start(&s, NS_RADIO_ESP, true, false, 6000) == NS_OK);
    line = "BLE unknown-format line\n";
    assert(ns_session_feed(&s, (const uint8_t*)line, strlen(line), 6001) == NS_OK);
    assert(s.lines == 1 && s.ap_count == 0 && !s.observations);
    ns_session_stop(&s, 6002);
    assert(ns_session_start(&s, NS_RADIO_NRF, false, true, 7000) == NS_OK);
    assert(ns_session_log_open(&s, "/ext/apps_data/netsentinel/session_002.csv") == NS_OK);
    m.fail_write = true;
    assert(ns_session_tick(&s, 7001) == NS_IO && !m.owner && !s.logging && m.closes == 2);
    m.fail_write = false;
    for(unsigned i = 0; i < 20; i++) {
        assert(ns_session_start(&s, NS_RADIO_CC, false, true, 8000+i) == NS_OK);
        assert(ns_cc_tune(&s.cc, 433920000) == NS_OK);
        ns_session_stop(&s, 8001+i);
    }
    assert(s.history_count == NS_HISTORY_LIMIT && m.acquired == m.released);
    assert(ns_session_start(&s, NS_RADIO_CC, false, true, 9000) == NS_OK);
    assert(ns_session_log_open(&s, "/ext/apps_data/netsentinel/sync.csv") == NS_OK);
    m.fail_sync = true; assert(ns_session_log_close(&s) == NS_IO && !s.logging);
    ns_session_stop(&s, 9001);
    m.fail_uart = true;
    assert(ns_session_start(&s, NS_RADIO_ESP, false, false, 0) == NS_IO && !m.owner);
    assert(m.acquired == m.released);
}
static void test_settings(void) {
    NsSettings defaults = ns_settings_default(), parsed = defaults;
    char text[100];
    assert(ns_settings_format(&defaults, text, sizeof(text)) == NS_OK);
    assert(ns_settings_parse(text, &parsed) == NS_OK);
    assert(parsed.cc_frequency_hz == 433920000);
    assert(ns_settings_parse("NS1 115200 5000 169 433920000", &parsed) == NS_INVALID);
    assert(ns_settings_parse("NS1 115200 5000 200 350000000", &parsed) == NS_INVALID);
    assert(ns_settings_parse("NS1 115200 5000 200 433920000 evil", &parsed) == NS_INVALID);
    assert(ns_settings_parse("NS1 99999999999999999999 5000 200 433920000", &parsed) == NS_INVALID);
    assert(parsed.baud == defaults.baud);
    assert(ns_settings_format(&defaults, text, 4) == NS_FULL);
    assert(ns_graph_height(1, 2, 40) == 20);
    assert(ns_graph_height(UINT32_MAX, UINT32_MAX, 40) == 40);
    assert(ns_graph_height(1, 0, 40) == 0);
}
int main(void) { test_observation_counter_limits(); test_settings(); test_spi_fault_matrix(); test_nrf(); test_cc(); test_session(); puts("PASS: receive safety, SPI/UART/storage failures, bounds, session cleanup and recovery"); }
