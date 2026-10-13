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
    uint8_t invalid_out[1]; size_t invalid_count = 99;
    assert(f.platform.uart_read(&f, NULL, 1, &invalid_count) == NS_INVALID && invalid_count == 0);
    invalid_count = 99;
    assert(f.platform.uart_read(&f, invalid_out, 0, &invalid_count) == NS_INVALID && invalid_count == 0);
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
    NsFrame frame; ns_app_frame(&app, &frame);
    assert(!strcmp(frame.rows[5], "Waiting for info reply"));
    const char* info = "#info\nVersion: v1.11.0\nHardware: Marauder v4\nESP-IDF: v5.5.1-710-g8410210c9a\n> ";
    mock_serial_feed((const uint8_t*)info, strlen(info));
    assert(ns_app_step(&app, 10) == NS_INVALID && !f.serial && !app.history_dirty);
    ns_app_frame(&app, &frame); assert(!strcmp(frame.rows[5], "Reply/state invalid"));
    app.status = NS_TIMEOUT; ns_app_frame(&app, &frame); assert(!strcmp(frame.rows[5], "No reply: check UART"));
    app.status = NS_BUSY; ns_app_frame(&app, &frame); assert(!strcmp(frame.rows[5], "Port/session busy"));
    app.status = NS_FULL; ns_app_frame(&app, &frame); assert(!strcmp(frame.rows[5], "UART buffer/log full"));
    app.status = NS_OK;
    assert(app.saved.history_count == 1 && app.saved.history[0].observations == 0);
    ns_app_frame(&app, &frame);
    assert(!strcmp(frame.rows[2], "Identity not validated"));
    assert(!strcmp(mock_serial_commands(), "info\n"));
    /* Explicit test-only verification: production initialization stays false. */
    f.capabilities.esp_wifi_verified = app.capabilities.esp_wifi_verified = app.session.installed_wifi_verified = true;
    assert(ns_app_key(&app, NS_KEY_OK, 20) == NS_OK);
    const char* full="#info\nFirmware: Marauder\nVersion: v1.11.0\nHardware: Marauder v4\nESP-IDF: v5.5.1-710-g8410210c9a\nWSL Bypass: enabled\nSD Card: Connected\nSD Card Size: 4096MB\nBattery Monitor: not supported\n> ";
    mock_serial_feed((const uint8_t*)full, strlen(full)); assert(ns_app_step(&app, 30) == NS_OK);
    const char* help = "#help\nsniffbeacon\nstopscan\n> ";
    mock_serial_feed((const uint8_t*)help, strlen(help)); assert(ns_app_step(&app, 40) == NS_OK);
    const char* start = "#sniffbeacon\nStartingBeacon sniff. Stop with stopscan\n> ";
    mock_serial_feed((const uint8_t*)start, strlen(start)); assert(ns_app_step(&app, 50) == NS_OK);
    const char* ap = "-65 Ch: 3 00:11:22:33:44:55 ESSID: Test network\n";
    mock_serial_feed((const uint8_t*)ap, strlen(ap)); assert(ns_app_step(&app, 60) == NS_OK);
    ns_app_frame(&app, &frame); assert(strstr(frame.rows[1], "identifiers hidden") && strstr(frame.rows[2], "Ch3 -65"));
    assert(ns_app_key(&app, NS_KEY_LEFT, 70) == NS_OK && app.baseline_count == 1);
    assert(ns_app_key(&app, NS_KEY_RIGHT, 70) == NS_OK); ns_app_frame(&app, &frame);
    assert(strstr(frame.rows[3], "hidden") && strstr(frame.rows[4], "not reported"));
    assert(ns_app_key(&app, NS_KEY_BACK, 80) == NS_BUSY && f.serial);
    assert(ns_app_key(&app, NS_KEY_BACK, 90) == NS_BUSY && app.running);
    const char* stop = "#stopscan\nStopping WiFi tran/recv\n> "; mock_serial_feed((const uint8_t*)stop, strlen(stop));
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
    assert(mock_sdk_queue_remaining()==0); /* Back drained stale input despite overflow */
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
/* Synthetic info transport faults: controller must release HAL ownership,
 * preserve error history and emit no scan/stop commands during info. */
static void test_info_transport_faults(void) {
    unsigned events[] = {FuriHalSerialRxEventFrameError, FuriHalSerialRxEventNoiseError,
        FuriHalSerialRxEventOverrunError, FuriHalSerialRxEventParityError};
    for(size_t i = 0; i < sizeof(events)/sizeof(events[0]); i++) {
        mock_sdk_reset(); NsFlipper f; NsApplication app;
        assert(ns_flipper_init(&f));
        assert(ns_app_init(&app, &f.platform, &f.store, f.capabilities) == NS_ABSENT);
        assert(ns_app_device_info(&app, 0) == NS_OK);
        mock_serial_feed((const uint8_t*)"#info\nVersion: v1.11.0\n", 23);
        mock_serial_error(events[i]);
        assert(ns_app_step(&app, 1) == NS_IO);
        assert(!f.serial && f.owner == NS_RADIO_NONE && !app.history_dirty);
        assert(app.saved.history_count == 1 && app.saved.history[0].result == NS_IO);
        assert(!strcmp(mock_serial_commands(), "info\n"));
        assert(ns_app_device_info(&app, 2) == NS_OK);
        assert(ns_app_stop(&app, 3) == NS_OK && !f.serial);
        assert(!strcmp(mock_serial_commands(), "info\n")); /* mock capture resets on reacquire */
        ns_flipper_deinit(&f); assert(!mock_sdk_live() && !mock_sdk_records());
    }
    mock_sdk_reset(); NsFlipper f; NsApplication app;
    assert(ns_flipper_init(&f));
    assert(ns_app_init(&app, &f.platform, &f.store, f.capabilities) == NS_ABSENT);
    assert(ns_app_device_info(&app, UINT32_MAX-499) == NS_OK);
    assert(ns_app_step(&app, 4499) == NS_OK && f.serial);
    assert(ns_app_step(&app, 4500) == NS_TIMEOUT && !f.serial);
    assert(!strcmp(mock_serial_commands(), "info\n"));
    ns_flipper_deinit(&f); assert(!mock_sdk_live() && !mock_sdk_records());
}
static void test_all_screen_restarts(void) {
    mock_sdk_reset();
    for(unsigned cycle=0;cycle<3;cycle++) for(unsigned menu=0;menu<NS_MENU_COUNT;menu++) {
        InputEvent events[24]; size_t n=0;
        for(unsigned i=0;i<menu;i++) events[n++]=(InputEvent){InputKeyDown,InputTypeShort};
        events[n++]=(InputEvent){InputKeyOk,InputTypeShort};
        if(menu==4 || menu==5 || menu==6 || menu==7) {
            events[n++]=(InputEvent){InputKeyDown,InputTypeRepeat};
            events[n++]=(InputEvent){InputKeyRight,InputTypeShort};
            events[n++]=(InputEvent){InputKeyLeft,InputTypeShort};
        }
        if(menu==8 || menu==9) events[n++]=(InputEvent){InputKeyOk,InputTypeShort};
        events[n++]=(InputEvent){InputKeyBack,InputTypeShort};
        events[n++]=(InputEvent){InputKeyBack,InputTypeLong};
        mock_sdk_events(events,n);
        assert(netsentinel_app(NULL)==0);
        assert(!mock_sdk_live() && !mock_sdk_records());
        assert(!strcmp(mock_serial_commands(),""));
    }
}
int main(void) { test_all_screen_restarts(); test_input_failures(); test_adapter(); test_shell(); test_uart(); test_info_transport_faults(); puts("PASS: Flipper shell/storage contract shims, exclusive CSV, GUI lifecycle and allocation unwind (not SDK ABI)"); }
