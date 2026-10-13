#include "ns_flipper.h"
#include "mock_sdk.h"
#include "furi_hal_serial_control.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int32_t netsentinel_app(void*);
static void test_adapter(void) {
    mock_sdk_reset(); NsFlipper f; assert(ns_flipper_init(&f));
    assert(!f.capabilities.nrf && !f.capabilities.cc);
    assert(f.platform.acquire(f.platform.ctx, NS_RADIO_NRF) == NS_UNSUPPORTED);
    NsSavedState s; NsPersistence p;
    assert(ns_persistence_load(&p, &f.store, &s) == NS_ABSENT);
    s.settings.timeout_ms = 17000; assert(ns_persistence_save(&p, &s) == NS_OK);
    NsSavedState boot; NsPersistence q;
    assert(ns_persistence_load(&q, &f.store, &boot) == NS_OK && boot.settings.timeout_ms == 17000);
    const char* path = "/ext/apps_data/netsentinel/session_0000000001.csv";
    assert(f.platform.file_create(&f, "/ext/other.csv") == NS_INVALID);
    assert(f.platform.file_create(&f, path) == NS_OK);
    assert(f.platform.file_write(&f, "record\n", 7) == NS_OK);
    assert(f.platform.file_sync(&f) == NS_OK);
    f.platform.file_close(&f);
    assert(f.platform.file_create(&f, path) == NS_BUSY); /* no overwrite */
    mock_sdk_fail_storage(true);
    assert(ns_persistence_load(&q, &f.store, &boot) == NS_IO);
    ns_flipper_deinit(&f); assert(!mock_sdk_live() && !mock_sdk_records());
}
static void test_shell(void) {
    mock_sdk_reset();
    InputEvent events[] = {
        {InputKeyDown, InputTypeShort}, {InputKeyDown, InputTypeShort},
        {InputKeyDown, InputTypeShort}, {InputKeyDown, InputTypeShort},
        {InputKeyOk, InputTypeShort}, {InputKeyRight, InputTypeShort},
        {InputKeyBack, InputTypeShort}, {InputKeyBack, InputTypeShort}
    };
    mock_sdk_events(events, sizeof(events)/sizeof(events[0]));
    assert(netsentinel_app(NULL) == 0);
    assert(mock_sdk_draws() == 8 && !mock_sdk_live() && !mock_sdk_records());
    NsFlipper f; assert(ns_flipper_init(&f)); NsSavedState s; NsPersistence p;
    assert(ns_persistence_load(&p, &f.store, &s) == NS_OK && s.settings.timeout_ms == 6000);
    ns_flipper_deinit(&f);
    for(unsigned at = 1; at <= 4; at++) {
        mock_sdk_reset(); mock_sdk_fail_allocation(at);
        assert(netsentinel_app(NULL) == 1 && !mock_sdk_live() && !mock_sdk_records());
    }
    mock_sdk_reset(); mock_sdk_fail_storage(true);
    assert(netsentinel_app(NULL) == 1 && !mock_sdk_live() && !mock_sdk_records());
}
static void test_uart(void) {
    mock_sdk_reset(); NsFlipper f; assert(ns_flipper_init(&f));
    assert(f.capabilities.esp_uart && !f.capabilities.esp_wifi_verified);
    FuriHalSerialHandle* busy = furi_hal_serial_control_acquire(FuriHalSerialIdUsart);
    assert(busy && f.platform.acquire(&f, NS_RADIO_ESP) == NS_BUSY);
    furi_hal_serial_control_release(busy);
    assert(f.platform.acquire(&f, NS_RADIO_ESP) == NS_OK);
    assert(f.platform.acquire(&f, NS_RADIO_ESP) == NS_BUSY);
    assert(f.platform.uart_write(&f, (const uint8_t*)"info\n", 5) == NS_OK);
    assert(f.platform.uart_write(&f, (const uint8_t*)"sniffbeacon\n", 12) == NS_UNSUPPORTED);
    assert(f.platform.uart_write(&f, (const uint8_t*)"arbitrary\n", 10) == NS_UNSUPPORTED);
    assert(!strcmp(mock_serial_commands(), "info\n"));
    const uint8_t input[] = "fragment\r\n> "; uint8_t out[32]; size_t count;
    mock_serial_feed(input, sizeof(input)-1);
    assert(f.platform.uart_read(&f, out, 3, &count) == NS_OK && count == 3 && !memcmp(out, input, 3));
    assert(f.platform.uart_read(&f, out, sizeof(out), &count) == NS_OK && count == sizeof(input)-4);
    /* Exercise wrap and exact capacity, then require a poisoned stream on loss. */
    uint8_t fill[NS_UART_CAPACITY]; memset(fill, 'x', sizeof(fill));
    mock_serial_feed(fill, sizeof(fill));
    assert(f.platform.uart_read(&f, out, sizeof(out), &count) == NS_OK && count == sizeof(out));
    mock_serial_feed(fill, sizeof(out)+1);
    assert(f.platform.uart_read(&f, out, sizeof(out), &count) == NS_FULL && !count);
    f.platform.release(&f, NS_RADIO_ESP);
    assert(f.platform.acquire(&f, NS_RADIO_ESP) == NS_OK);
    mock_serial_error(FuriHalSerialRxEventFrameError);
    assert(f.platform.uart_read(&f, out, sizeof(out), &count) == NS_IO && !count);
    ns_flipper_deinit(&f); assert(!mock_sdk_live() && !mock_sdk_records());
    mock_sdk_reset(); assert(ns_flipper_init(&f)); NsApplication app;
    assert(ns_app_init(&app, &f.platform, &f.store, f.capabilities) == NS_ABSENT);
    app.selected = NS_RADIO_ESP; app.page = NS_PAGE_RADIO;
    assert(ns_app_key(&app, NS_KEY_OK, 0) == NS_UNSUPPORTED && !f.serial);
    assert(ns_app_key(&app, NS_KEY_RIGHT, 0) == NS_OK && f.serial);
    const char* info = "#info\nVersion: v1.11.0\nHardware: Marauder v4\nESP-IDF: v5.5.1-710-g840210c9a\n> ";
    mock_serial_feed((const uint8_t*)info, strlen(info));
    assert(ns_app_step(&app, 10) == NS_OK && !f.serial && !app.history_dirty);
    assert(app.saved.history_count == 1 && app.saved.history[0].observations == 0);
    NsFrame frame; ns_app_frame(&app, &frame);
    assert(!strcmp(frame.rows[2], "v1.11.0"));
    assert(!strcmp(mock_serial_commands(), "info\n"));
    /* Explicit test-only verification: production initialization stays false. */
    f.capabilities.esp_wifi_verified = app.capabilities.esp_wifi_verified = app.session.installed_wifi_verified = true;
    assert(ns_app_key(&app, NS_KEY_OK, 20) == NS_OK);
    mock_serial_feed((const uint8_t*)info, strlen(info)); assert(ns_app_step(&app, 30) == NS_OK);
    const char* help = "#help\nsniffbeacon\nstopscan\n> ";
    mock_serial_feed((const uint8_t*)help, strlen(help)); assert(ns_app_step(&app, 40) == NS_OK);
    const char* start = "#sniffbeacon\nStartingBeacon sniff. Stop with stopscan\n> ";
    mock_serial_feed((const uint8_t*)start, strlen(start)); assert(ns_app_step(&app, 50) == NS_OK);
    const char* ap = "-65 Ch: 3 00:11:22:33:44:55 ESSID: Test network\n";
    mock_serial_feed((const uint8_t*)ap, strlen(ap)); assert(ns_app_step(&app, 60) == NS_OK);
    ns_app_frame(&app, &frame); assert(!strcmp(frame.rows[1], "Test network") && strstr(frame.rows[2], "Ch3 -65"));
    assert(ns_app_key(&app, NS_KEY_LEFT, 70) == NS_OK && app.baseline_count == 1);
    assert(ns_app_key(&app, NS_KEY_RIGHT, 70) == NS_OK); ns_app_frame(&app, &frame);
    assert(strstr(frame.rows[3], "00:11:22:33:44:55") && strstr(frame.rows[4], "not reported"));
    assert(ns_app_key(&app, NS_KEY_BACK, 80) == NS_BUSY && f.serial);
    assert(ns_app_key(&app, NS_KEY_BACK, 90) == NS_BUSY && app.running);
    const char* stop = "#stopscan\n> "; mock_serial_feed((const uint8_t*)stop, strlen(stop));
    assert(ns_app_step(&app, 100) == NS_OK && !app.running && !f.serial && app.saved.history_count == 2);
    ns_flipper_deinit(&f); assert(!mock_sdk_live() && !mock_sdk_records());
}
static void test_input_failures(void) {
    mock_sdk_reset(); InputEvent ignored[] = {
        {InputKeyDown, InputTypePress}, {InputKeyOk, InputTypeRepeat}, {InputKeyBack, InputTypeLong}};
    mock_sdk_events(ignored, 3); assert(netsentinel_app(NULL) == 0 && mock_sdk_draws() == 3);
    assert(!mock_sdk_live() && !mock_sdk_records());
    mock_sdk_reset(); InputEvent flood[25];
    for(unsigned i=0; i<24; i++) flood[i] = (InputEvent){InputKeyDown, InputTypeRepeat};
    flood[24] = (InputEvent){InputKeyBack, InputTypeShort};
    mock_sdk_events(flood, 25); mock_sdk_burst(true);
    assert(netsentinel_app(NULL) == 0 && mock_sdk_queue_drops() == 8 && !mock_sdk_live() && !mock_sdk_records());
    mock_sdk_reset(); mock_sdk_fail_queue(true);
    mock_sdk_events(ignored, 3); assert(netsentinel_app(NULL) == 0 && !mock_sdk_live() && !mock_sdk_records());
    const char* records[] = {RECORD_GUI, RECORD_STORAGE};
    for(unsigned i=0; i<2; i++) {
        mock_sdk_reset(); mock_sdk_fail_record(records[i]);
        assert(netsentinel_app(NULL) == 1 && !mock_sdk_live() && !mock_sdk_records());
    }
    mock_sdk_reset(); mock_sdk_fail_mutex(1);
    assert(netsentinel_app(NULL) == 1 && !mock_sdk_live() && !mock_sdk_records());
}
int main(void) { test_input_failures(); test_adapter(); test_shell(); test_uart(); puts("PASS: Flipper shell/storage contract shims, exclusive CSV, GUI lifecycle and allocation unwind (not SDK ABI)"); }
