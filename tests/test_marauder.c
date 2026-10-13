#include "netsentinel/application.h"
#include "mock_platform.h"
#include "ns_analysis.h"
#include <stdio.h>
/* Identity labels match tagged upstream; values are synthetic, not a device capture. */
static const char* identity = "#info\r\nFirmware: Marauder\r\nVersion: v1.11.0\r\nHardware: Marauder v4\r\nESP-IDF: v5.5.1-710-g8410210c9a\r\nWSL Bypass: enabled\r\nSD Card: Connected\r\nSD Card Size: 4096MB\r\nBattery Monitor: not supported\r\n> ";
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
    assert(strstr(m.file, "uart_redacted") && !strstr(m.file, "Guest"));
    uint8_t counts[14]; ns_channel_counts(s.aps, s.ap_count, counts); assert(counts[0] == 1 && counts[5] == 1 && counts[10] == 1);
    assert(ns_session_cancel(&s, 50) == NS_BUSY && s.active == NS_RADIO_ESP);
    assert(ns_session_cancel(&s, 51) == NS_BUSY);
    feed(&s, "#stopscan\nStopping WiFi tran/recv\n> ", 60);
    assert(s.active == NS_RADIO_NONE && m.released == 1 && m.closes == 1 && s.history_count == 1);
    assert(!strcmp(m.commands, "info\nhelp\nsniffbeacon\nstopscan\n"));
    assert(strstr(m.file, "wifi_channel,,6,1,APs"));
}
static void observation_limits(void) {
    Mock m={0};NsPlatform io=platform(&m);NsSession s;open_scan(&s,&io);
    assert(ns_session_log_open(&s,"/ext/apps_data/netsentinel/session_0000000001.csv")==NS_OK);
    const char* ap="-50 Ch: 6 00:11:22:33:44:55 ESSID: PRIVATE_SYNTHETIC_AP\n";
    assert(ns_session_feed(&s,(const uint8_t*)ap,strlen(ap),40)==NS_OK);
    assert(strstr(m.file,"wifi_ap,ap_1,6,-50,dBm") && !strstr(m.file,"PRIVATE") && !strstr(m.file,"00:11"));
    m.fail_write=true;
    assert(ns_session_feed(&s,(const uint8_t*)ap,strlen(ap),41)==NS_IO && !s.active && !m.owner);
    memset(&m,0,sizeof(m));open_scan(&s,&io);
    s.observations=UINT32_MAX;
    assert(ns_session_feed(&s,(const uint8_t*)ap,strlen(ap),40)==NS_FULL && !s.active && !m.owner);
    assert(s.observations==UINT32_MAX && !s.ap_count && s.history[0].result==NS_FULL);
    memset(&m,0,sizeof(m));open_scan(&s,&io);s.ap_count=NS_AP_LIMIT;
    for(unsigned j=0;j<NS_AP_LIMIT;j++) { strcpy(s.aps[j].ssid,"other");s.aps[j].channel=1; }
    assert(ns_session_feed(&s,(const uint8_t*)ap,strlen(ap),40)==NS_FULL && !s.active && !m.owner);
    assert(s.history_count==1 && s.history[0].result==NS_FULL && m.acquired==m.released);
}
static void source_verified_states(void) {
    const char* no_sd="#info\nFirmware: Marauder\nVersion: v1.11.0\nHardware: Marauder v4\nESP-IDF: v5.5.1-710-g8410210c9a\nWSL Bypass: disabled\nSD Card: Not Connected\nSD Card Size: 0\nBattery Monitor: not supported\n> ";
    Mock m={0};NsPlatform io=platform(&m);NsSession s;ns_session_init(&s,&io,NULL);s.use_marauder=true;
    assert(ns_session_start(&s,NS_RADIO_ESP,false,false,0)==NS_OK);feed(&s,no_sd,1);
    assert(s.marauder.info_complete && s.marauder.identity_matches && !s.marauder.sd_connected && !s.marauder.wsl_bypass && !m.owner);
    memset(&m,0,sizeof(m));ns_session_init(&s,&io,NULL);s.use_marauder=true;
    assert(ns_session_start(&s,NS_RADIO_ESP,false,false,0)==NS_OK);
    feed(&s,"#info\nFirmware: Marauder\nVersion: v1.11.0\nHardware: Marauder v4\nESP-IDF: v5.5.1-710-g8410210c9a\nSD Card: Connected\nSD Card Size: 0\n",1);
    assert(ns_session_feed(&s,(const uint8_t*)"> ",2,2)==NS_INVALID && !m.owner);
    memset(&m,0,sizeof(m));open_scan(&s,&io);
    assert(ns_session_cancel(&s,40)==NS_BUSY);
    const char* echo_only="#stopscan\n> ";
    assert(ns_session_feed(&s,(const uint8_t*)echo_only,strlen(echo_only),41)==NS_INVALID && !m.owner && !s.marauder.stop_confirmed);
    memset(&m,0,sizeof(m));open_scan(&s,&io);
    assert(ns_session_cancel(&s,40)==NS_BUSY);
    feed(&s,"#stopscan\nStopping WiFi tran/recv\n> ",41);
    assert(s.marauder.stop_confirmed && !m.owner);
    memset(&m,0,sizeof(m));ns_session_init(&s,&io,NULL);s.use_marauder=s.wifi_request=s.installed_wifi_verified=true;
    assert(ns_session_start(&s,NS_RADIO_ESP,false,false,0)==NS_OK);
    feed(&s,"#info\nVersion: v1.11.0\nHardware: Marauder v4\nESP-IDF: v5.5.1-710-g8410210c9a\n",1);
    assert(ns_session_feed(&s,(const uint8_t*)"> ",2,2)==NS_INVALID && !m.owner && !strstr(m.commands,"help"));
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
    assert(ns_marauder_line(&p, "ESP-IDF: v5.5.1-710-g8410210c9a", 1) == NS_OK);
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
    assert(ns_marauder_line(&p, "ESP-IDF: v5.5.1-710-g8410210c9a", 0) == NS_OK);
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
    assert(ns_session_feed(&s, nul_line, sizeof(nul_line), 1) == NS_INVALID);
    assert(s.dropped_lines == 0 && !m.owner && m.released == 1);
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
static void observed_info_fixture(char* out, size_t capacity) {
    FILE* f = fopen("tests/fixtures/marauder_info_sanitized.txt", "r"); assert(f);
    out[0] = 0; char line[256];
    while(fgets(line, sizeof(line), f)) {
        if(line[0] == '#') continue;
        assert(strlen(out)+strlen(line) < capacity); strcat(out, line);
    }
    assert(!ferror(f)); fclose(f);
}
static void observed_info_status(void) {
    char body[1024]; observed_info_fixture(body, sizeof(body));
    for(unsigned crlf = 0; crlf < 2; crlf++) {
        char reply[2048]; size_t n = 0;
        const char* echo = "#info\n";
        for(unsigned pass = 0; pass < 2; pass++) {
            const char* p = pass ? body : echo;
            for(; *p; p++) { if(crlf && *p == '\n') reply[n++] = '\r'; reply[n++] = *p; }
        }
        reply[n++] = '>'; reply[n++] = ' '; reply[n] = 0; /* synthetic transport framing */
        for(size_t split = 0; split <= n; split++) {
            Mock m = {0}; NsPlatform io = platform(&m); NsSession s;
            ns_session_init(&s, &io, NULL); s.use_marauder = true;
            assert(ns_session_start(&s, NS_RADIO_ESP, false, false, 0) == NS_OK);
            assert(ns_session_feed(&s, (const uint8_t*)reply, split, 1) == NS_OK);
            if(split < n) {
                assert(!s.marauder.info_complete && !s.marauder.identity_matches);
                assert(ns_session_feed(&s, (const uint8_t*)reply+split, n-split, 2) == NS_OK);
            }
            assert(s.marauder.info_complete && s.marauder.identity_matches && !s.active);
            assert(s.marauder.sd_connected && s.marauder.sd_size_mb == 30436);
            assert(s.marauder.battery_unsupported && s.marauder.wsl_bypass);
            assert(!strcmp(s.marauder.idf, "v5.5.1-710-g8410210c9a"));
            assert(!strcmp(m.commands, "info\n") && m.released == 1);
            NsApplication app = {0}; app.page = NS_PAGE_RADIO; app.selected = NS_RADIO_ESP;
            app.capabilities.esp_uart = true; app.session.marauder = s.marauder;
            NsFrame frame; ns_app_frame(&app, &frame);
            assert(!strcmp(frame.rows[5], "Info reply received"));
        }
    }
    const char* required[] = {"Firmware:","Version:","Hardware:","ESP-IDF:",
        "WSL Bypass:","SD Card:","SD Card Size:","Battery Monitor:"};
    for(size_t missing = 0; missing < sizeof(required)/sizeof(required[0]); missing++) {
        char incomplete[1024]; strcpy(incomplete, body);
        char* line = strstr(incomplete, required[missing]); assert(line);
        char* end = strchr(line, '\n'); assert(end); memmove(line, end+1, strlen(end+1)+1);
        Mock m = {0}; NsPlatform io = platform(&m); NsSession s;
        ns_session_init(&s, &io, NULL); s.use_marauder = true;
        assert(ns_session_start(&s, NS_RADIO_ESP, false, false, 0) == NS_OK);
        feed(&s, "#info\n", 1); feed(&s, incomplete, 2);
        NsResult r = ns_session_feed(&s, (const uint8_t*)"> ", 2, 3);
        assert(r == NS_INVALID && s.history[0].result == NS_INVALID);
        assert(!s.marauder.info_complete && !s.marauder.identity_matches && !s.active);
        NsApplication app = {0}; app.page = NS_PAGE_RADIO; app.selected = NS_RADIO_ESP;
        app.capabilities.esp_uart = true; app.session.marauder = s.marauder; app.status = r;
        NsFrame frame; ns_app_frame(&app, &frame);
        assert(strcmp(frame.rows[5], "Info reply received"));
    }
    Mock m = {0}; NsPlatform io = platform(&m); NsSession s;
    ns_session_init(&s, &io, NULL); s.use_marauder = true;
    assert(ns_session_start(&s, NS_RADIO_ESP, false, false, 0) == NS_OK);
    assert(ns_session_log_open(&s, "/ext/apps_data/netsentinel/session_0000000001.csv") == NS_OK);
    feed(&s, "#info\n", 1);
    feed(&s, "Station MAC: PRIVATE_TEST_STATION\nAP MAC: PRIVATE_TEST_AP\nSerial Number: PRIVATE_TEST_SERIAL\nFuture Field: new\n", 2);
    feed(&s, body, 3); feed(&s, "> ", 4);
    assert(s.marauder.info_complete && !strstr(m.file, "PRIVATE_TEST") && !strstr(m.file, "Station MAC"));
    assert(!strstr(m.file, "30436") && !strstr(m.file, "v1.11.0") && strstr(m.file, "uart_redacted"));
    assert(m.closes == 1 && m.released == 1);
    memset(&m, 0, sizeof(m)); ns_session_init(&s, &io, NULL); s.use_marauder = true;
    assert(ns_session_start(&s, NS_RADIO_ESP, false, false, 0) == NS_OK);
    feed(&s, "#info\n", 1); feed(&s, body, 2);
    assert(!s.marauder.info_complete);
    assert(ns_session_tick(&s, 5000) == NS_TIMEOUT && !s.marauder.identity_matches);
    memset(&m, 0, sizeof(m)); ns_session_init(&s, &io, NULL); s.use_marauder = true;
    assert(ns_session_start(&s, NS_RADIO_ESP, false, false, 0) == NS_OK);
    feed(&s, body, 1); feed(&s, "> ", 2); /* uncorrelated echo-free body is not verification */
    assert(!s.marauder.info_complete && !s.marauder.identity_matches);
    assert(ns_session_tick(&s, 5000) == NS_TIMEOUT && !m.owner);
}
static void observed_info_malformed(void) {
    const char* bad[] = {"SD Card Size: 4294967296MB", "SD Card Size: -1MB",
        "SD Card Size: 0MB", "SD Card Size: 30436MBjunk", "Battery Monitor: mystery",
        "WSL Bypass: mystery", "SD Card: mystery", "Version: v1.11.0?"};
    for(size_t i = 0; i < sizeof(bad)/sizeof(bad[0]); i++) {
        Mock m = {0}; NsPlatform io = platform(&m); NsMarauder p;
        assert(ns_marauder_begin(&p, &io, false, false, 5000, 0) == NS_OK);
        assert(ns_marauder_line(&p, "#info", 1) == NS_OK);
        assert(ns_marauder_line(&p, bad[i], 2) == NS_INVALID);
        assert(!p.info_complete && !p.identity_matches && p.stage == NS_M_FAILED);
    }
    Mock m = {0}; NsPlatform io = platform(&m); NsMarauder p;
    assert(ns_marauder_begin(&p, &io, false, false, 5000, 0) == NS_OK);
    assert(ns_marauder_line(&p, "#info", 1) == NS_OK);
    assert(ns_marauder_line(&p, "Version: v1.11.0", 2) == NS_OK);
    assert(ns_marauder_line(&p, "Version: v1.11.0", 3) == NS_INVALID); /* duplicate fails closed */
    assert(!p.info_complete);
}
int main(void) { source_verified_states(); observation_limits(); protocol(); failures(); rejection(); information_deadlines(); info_fragment_boundaries(); info_malformed_and_retry(); info_disconnect_and_cancel(); observed_info_status(); observed_info_malformed(); puts("PASS: gated Marauder protocol, byte fragments, fixture observations, cancellation, logging and timeout recovery"); }
