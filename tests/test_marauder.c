#include "netsentinel/application.h"
#include "mock_platform.h"
#include "ns_analysis.h"
#include <stdio.h>
/* Identity labels match tagged upstream; values are synthetic, not a device capture. */
static const char* identity = "#info\r\nVersion: v1.11.0\r\nHardware: Marauder v4\r\nESP-IDF: v5.5.1-710-g840210c9a\r\n> ";
static void feed(NsSession* s, const char* text, uint32_t now) {
    for(size_t i=0; text[i]; i++) assert(ns_session_feed(s, (const uint8_t*)text+i, 1, now) == NS_OK);
}
static void open_scan(NsSession* s, NsPlatform* io) {
    ns_session_init(s, io, NULL); s->use_marauder = s->wifi_request = s->installed_wifi_verified = true;
    assert(ns_session_start(s, NS_RADIO_ESP, false, false, 0) == NS_OK);
    feed(s, identity, 10);
    feed(s, "#help\nsniffbeacon description\nstopscan description\n> ", 20);
    feed(s, "#sniffbeacon\nStartingBeacon sniff. Stop with stopscan\n> ", 30);
    assert(s->marauder.stage == NS_M_SCAN);
}
static void protocol(void) {
    Mock m = {0}; NsPlatform io = platform(&m); NsSession s;
    ns_session_init(&s, &io, NULL); s.use_marauder = s.wifi_request = true;
    assert(ns_session_start(&s, NS_RADIO_ESP, false, false, 0) == NS_UNSUPPORTED && !m.acquired && !m.uart_calls);
    s.wifi_request = false;
    assert(ns_session_start(&s, NS_RADIO_ESP, true, false, 0) == NS_UNSUPPORTED);
    assert(ns_session_start(&s, NS_RADIO_ESP, false, false, 0) == NS_OK);
    feed(&s, "> ", 1); assert(s.marauder.stage == NS_M_INFO); /* stale boot prompt */
    feed(&s, identity, 10);
    assert(s.active == NS_RADIO_NONE && !strcmp(m.commands, "info\n") && !strcmp(s.marauder.hardware, "Marauder v4"));
    memset(&m, 0, sizeof(m)); open_scan(&s, &io);
    assert(!strcmp(m.commands, "info\nhelp\nsniffbeacon\n"));
    assert(ns_session_log_open(&s, "/ext/apps_data/netsentinel/session_0000000001.csv") == NS_OK);
    FILE* fixture = fopen("tests/fixtures/marauder_beacons.txt", "r"); assert(fixture);
    char line[256]; while(fgets(line, sizeof(line), fixture)) feed(&s, line, 40);
    assert(!ferror(fixture)); fclose(fixture);
    assert(s.ap_count == 3 && s.observations == 4 && s.aps[0].rssi == -44 && s.aps[0].channel == 6);
    assert(s.ap_min[0] == -59 && s.ap_max[0] == -44 && s.ap_seen_ms[0] == 40);
    assert(!strcmp(s.aps[2].ssid, "<hidden>") && !s.aps[0].security[0]);
    assert(strstr(m.file, "Guest,\"\"net\"\""));
    uint8_t counts[14]; ns_channel_counts(s.aps, s.ap_count, counts); assert(counts[0] == 1 && counts[5] == 1 && counts[10] == 1);
    assert(ns_session_cancel(&s, 50) == NS_BUSY && s.active == NS_RADIO_ESP);
    assert(ns_session_cancel(&s, 51) == NS_BUSY);
    feed(&s, "#stopscan\n> ", 60);
    assert(s.active == NS_RADIO_NONE && m.released == 1 && m.closes == 1 && s.history_count == 1);
    assert(!strcmp(m.commands, "info\nhelp\nsniffbeacon\nstopscan\n"));
    assert(strstr(m.file, "wifi_channel,,6,1,APs"));
}
static void failures(void) {
    Mock m = {0}; NsPlatform io = platform(&m); NsSession s;
    open_scan(&s, &io);
    char huge[400]; memset(huge, 'x', sizeof(huge));
    assert(ns_session_feed(&s, (const uint8_t*)huge, sizeof(huge), 40) == NS_OK);
    feed(&s, "\n-59 Ch: 3 00:11:22:33:44:55 ESSID: recovered\n", 45);
    assert(s.dropped_lines == 1 && s.ap_count == 1);
    feed(&s, "junk\n", 4900); /* junk cannot extend the AP deadline */
    assert(ns_session_tick(&s, 5045) == NS_TIMEOUT && !m.owner && m.released == 1);
    assert(strstr(m.commands, "stopscan\n"));
    memset(&m, 0, sizeof(m)); ns_session_init(&s, &io, NULL); s.use_marauder = true;
    assert(ns_session_start(&s, NS_RADIO_ESP, false, false, UINT32_MAX-100) == NS_OK);
    feed(&s, "partial", 10);
    assert(ns_session_tick(&s, 4899) == NS_TIMEOUT && !m.owner); /* unsigned wrap */
    memset(&m, 0, sizeof(m)); ns_session_init(&s, &io, NULL); s.use_marauder = true;
    assert(ns_session_start(&s, NS_RADIO_ESP, false, false, 0) == NS_OK);
    assert(ns_session_feed(&s, (const uint8_t*)huge, sizeof(huge), 1) == NS_OK);
    assert(ns_session_feed(&s, (const uint8_t*)"\n", 1, 1) == NS_FULL && !m.owner);
    memset(&m, 0, sizeof(m)); open_scan(&s, &io);
    assert(ns_session_cancel(&s, 100) == NS_BUSY);
    assert(ns_session_tick(&s, 5100) == NS_TIMEOUT && !m.owner);
    memset(&m, 0, sizeof(m)); m.fail_uart = true; ns_session_init(&s, &io, NULL); s.use_marauder = true;
    assert(ns_session_start(&s, NS_RADIO_ESP, false, false, 0) == NS_IO && m.released == 1);
}
static void rejection(void) {
    Mock m = {0}; NsPlatform io = platform(&m); NsMarauder p;
    assert(ns_marauder_begin(&p, &io, true, false, 5000, 0) == NS_UNSUPPORTED && !m.uart_calls);
    assert(ns_marauder_begin(&p, &io, true, true, 5000, 0) == NS_OK);
    assert(ns_marauder_line(&p, "#info", 1) == NS_OK);
    assert(ns_marauder_line(&p, "Version: v1.10.0", 1) == NS_OK);
    assert(ns_marauder_line(&p, "Hardware: Marauder v4", 1) == NS_OK);
    assert(ns_marauder_line(&p, "ESP-IDF: v5.5.1-710-g840210c9a", 1) == NS_OK);
    assert(ns_marauder_prompt(&p, 1) == NS_UNSUPPORTED && !strstr(m.commands, "sniffbeacon"));
    NsSession s; memset(&m, 0, sizeof(m)); ns_session_init(&s, &io, NULL);
    s.use_marauder = s.wifi_request = s.installed_wifi_verified = true;
    assert(ns_session_start(&s, NS_RADIO_ESP, false, false, 0) == NS_OK); feed(&s, identity, 10);
    feed(&s, "#help\nsniffbeacon_only\nstopscan\n", 20);
    assert(ns_session_feed(&s, (const uint8_t*)"> ", 2, 20) == NS_UNSUPPORTED && !m.owner);
}
static void information_deadlines(void) {
    Mock m = {0}; NsPlatform io = platform(&m); NsMarauder p;
    assert(ns_marauder_begin(&p, &io, false, false, 1000, 0) == NS_OK);
    assert(ns_marauder_line(&p, "#info", 1) == NS_OK);
    assert(ns_marauder_line(&p, "Version: v1.11.0", 999) == NS_OK);
    assert(ns_marauder_line(&p, "Hardware: Marauder v4", 1000) == NS_TIMEOUT);
    assert(p.stage == NS_M_FAILED && !p.hardware[0]);
    assert(ns_marauder_prompt(&p, 1001) == NS_TIMEOUT);
    assert(!strcmp(m.commands, "info\n"));
    memset(&m, 0, sizeof(m));
    assert(ns_marauder_begin(&p, &io, false, false, 1000, UINT32_MAX-99) == NS_OK);
    assert(ns_marauder_line(&p, "#info", UINT32_MAX-98) == NS_OK);
    assert(ns_marauder_line(&p, "Version: v1.11.0", 0) == NS_OK);
    assert(ns_marauder_line(&p, "Hardware: Marauder v4", 0) == NS_OK);
    assert(ns_marauder_line(&p, "ESP-IDF: v5.5.1-710-g840210c9a", 0) == NS_OK);
    assert(ns_marauder_prompt(&p, 900) == NS_TIMEOUT); /* exact wrapped deadline */
    assert(!strcmp(m.commands, "info\n"));
    memset(&m, 0, sizeof(m));
    assert(ns_marauder_begin(&p, &io, false, false, 1000, 0) == NS_OK);
    assert(ns_marauder_cancel(&p, 1) == NS_OK && p.stage == NS_M_DONE);
    assert(ns_marauder_abort(&p) == NS_OK && !strcmp(m.commands, "info\n"));
}
/* Exhaust every two-fragment boundary of the upstream-label synthetic info
 * reply, including CRLF, echo and prompt. No physical recording is used. */
static void info_fragment_boundaries(void) {
    size_t n = strlen(identity);
    for(size_t split = 0; split <= n; split++) {
        Mock m = {0}; NsPlatform io = platform(&m); NsSession s;
        ns_session_init(&s, &io, NULL); s.use_marauder = true;
        assert(ns_session_start(&s, NS_RADIO_ESP, false, false, 0) == NS_OK);
        assert(ns_session_feed(&s, (const uint8_t*)identity, split, 10) == NS_OK);
        if(s.active != NS_RADIO_NONE)
            assert(ns_session_feed(&s, (const uint8_t*)identity+split, n-split, 11) == NS_OK);
        assert(s.active == NS_RADIO_NONE && s.marauder.stage == NS_M_DONE);
        assert(!strcmp(s.marauder.version, "v1.11.0"));
        assert(m.acquired == 1 && m.released == 1 && !strcmp(m.commands, "info\n"));
    }
}
static void info_malformed_and_retry(void) {
    const char* bad[] = {
        "#info\nVersion: v1.11.0\nHardware: Marauder v4\n> ", /* missing IDF */
        "#info\nVersion: v1.11.0xxxxxxxxxxxxxxxxxxxxxxxx\nHardware: Marauder v4\nESP-IDF: test\n> ",
        "#info\nVersion: \nHardware: Marauder v4\nESP-IDF: test\n> "
    };
    for(size_t i = 0; i < sizeof(bad)/sizeof(bad[0]); i++) {
        Mock m = {0}; NsPlatform io = platform(&m); NsSession s;
        ns_session_init(&s, &io, NULL); s.use_marauder = true;
        assert(ns_session_start(&s, NS_RADIO_ESP, false, false, 0) == NS_OK);
        assert(ns_session_feed(&s, (const uint8_t*)bad[i], strlen(bad[i]), 1) == NS_INVALID);
        assert(!m.owner && m.released == 1 && !strcmp(m.commands, "info\n"));
        assert(ns_session_start(&s, NS_RADIO_ESP, false, false, 2) == NS_OK);
        feed(&s, identity, 3);
        assert(s.marauder.stage == NS_M_DONE && m.released == 2);
        assert(!strcmp(m.commands, "info\ninfo\n"));
    }
    Mock m = {0}; NsPlatform io = platform(&m); NsSession s;
    ns_session_init(&s, &io, NULL); s.use_marauder = true;
    assert(ns_session_start(&s, NS_RADIO_ESP, false, false, 0) == NS_OK);
    const uint8_t nul_line[] = {'#','i','n','f','o','\n','V',0,'x','\n'};
    assert(ns_session_feed(&s, nul_line, sizeof(nul_line), 1) == NS_FULL);
    assert(s.dropped_lines == 1 && !m.owner && m.released == 1);
    assert(ns_session_start(&s, NS_RADIO_ESP, false, false, 2) == NS_OK);
    uint8_t oversized[1024]; memset(oversized, 'x', sizeof(oversized));
    assert(ns_session_feed(&s, oversized, sizeof(oversized), 3) == NS_OK);
    assert(ns_session_feed(&s, (const uint8_t*)"\n", 1, 4) == NS_FULL);
    assert(!m.owner && m.released == 2 && !strcmp(m.commands, "info\ninfo\n"));
}
static void info_disconnect_and_cancel(void) {
    Mock m = {0}; NsPlatform io = platform(&m); NsSession s;
    ns_session_init(&s, &io, NULL); s.use_marauder = true;
    s.timeout_ms = 1000;
    uint32_t started = UINT32_MAX-499;
    assert(ns_session_start(&s, NS_RADIO_ESP, false, false, started) == NS_OK);
    /* Cable loss cannot be distinguished from silence by UART. */
    feed(&s, "#info\nVersion: v1.11.0\n", UINT32_MAX-100);
    assert(ns_session_tick(&s, 499) == NS_OK && m.owner == NS_RADIO_ESP);
    assert(ns_session_tick(&s, 500) == NS_TIMEOUT && !m.owner);
    assert(s.history_count == 1 && s.history[0].result == NS_TIMEOUT);
    assert(!strcmp(m.commands, "info\n"));
    assert(ns_session_start(&s, NS_RADIO_ESP, false, false, 501) == NS_OK);
    assert(ns_session_feed(&s, NULL, 1, 502) == NS_INVALID && m.owner);
    assert(ns_session_feed(&s, NULL, 0, 502) == NS_OK && m.owner);
    feed(&s, "#info\nVersion: partial", 503);
    assert(ns_session_cancel(&s, 504) == NS_OK && !m.owner);
    assert(ns_session_cancel(&s, 505) == NS_OK && m.released == 2);
    assert(ns_session_feed(&s, (const uint8_t*)identity, strlen(identity), 506) == NS_INVALID);
    assert(!strcmp(m.commands, "info\ninfo\n"));
}
int main(void) { protocol(); failures(); rejection(); information_deadlines(); info_fragment_boundaries(); info_malformed_and_retry(); info_disconnect_and_cancel(); puts("PASS: gated Marauder protocol, byte fragments, fixture observations, cancellation, logging and timeout recovery"); }
