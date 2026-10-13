/* Mock HAL test of exactly the separately gated live-info configuration. */
#include "ns_flipper.h"
#include "mock_sdk.h"
#include "furi_hal_serial_control.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void check_frame(NsApplication* a, const char* state) {
    NsFrame f; ns_app_frame(a, &f); assert(!strcmp(f.rows[5], state));
}
static void fixture(char* out) {
    FILE* file = fopen("tests/fixtures/marauder_info_sanitized.txt", "r"); assert(file);
    strcpy(out, "#info\n"); char line[256];
    while(fgets(line, sizeof(line), file)) if(line[0] != '#') {
        assert(strlen(out)+strlen(line) < 1020); strcat(out, line);
    }
    assert(!ferror(file)); fclose(file); strcat(out, "> ");
}
int32_t netsentinel_app(void*);
static void repeated(void) {
    mock_sdk_reset(); NsFlipper f; NsApplication a; assert(ns_flipper_init(&f));
    assert(ns_app_init(&a,&f.platform,&f.store,f.capabilities)==NS_ABSENT);
    a.page=NS_PAGE_RADIO; a.selected=NS_RADIO_ESP;
    assert(ns_app_key(&a,NS_KEY_OK,0)==NS_OK);
    assert(ns_app_device_info(&a,0)==NS_BUSY && !f.serial && a.info_pending==2);
    assert(ns_app_stop(&a,0)==NS_BUSY && !a.info_pending);
    char reply[1024]; fixture(reply);
    for(unsigned i=0;i<8;i++) {
        uint32_t now=i*6000+1;
        assert(ns_app_device_info(&a,now)==NS_OK);
        assert(ns_app_device_info(&a,now)==NS_BUSY);
        if(i%4==0) { mock_serial_feed((const uint8_t*)reply,strlen(reply)); assert(ns_app_step(&a,now+1)==NS_OK); }
        else if(i%4==1) assert(ns_app_step(&a,now+5000)==NS_TIMEOUT);
        else if(i%4==2) { const char* bad="#info\nVersion: \n"; mock_serial_feed((const uint8_t*)bad,strlen(bad)); assert(ns_app_step(&a,now+1)==NS_INVALID); }
        else assert(ns_app_stop(&a,now+1)==NS_OK);
        assert(!f.serial && f.owner==NS_RADIO_NONE && !a.history_dirty);
        assert(!strcmp(mock_serial_commands(),"info\n"));
        assert(a.saved.history_count==i+1);
    }
    ns_flipper_deinit(&f); assert(!mock_sdk_live()&&!mock_sdk_records());
    mock_sdk_reset(); InputEvent events[]={{InputKeyOk,InputTypeShort},{InputKeyOk,InputTypeShort},
        {InputKeyUp,InputTypeRepeat},{InputKeyBack,InputTypeShort},{InputKeyBack,InputTypeShort}};
    mock_sdk_events(events,sizeof(events)/sizeof(*events));
    assert(netsentinel_app(NULL)==0 && !mock_sdk_live()&&!mock_sdk_records());
}
int main(void) {
    mock_sdk_reset(); NsFlipper f; assert(ns_flipper_init(&f));
    assert(f.capabilities.esp_info_only && f.capabilities.esp_uart);
    assert(!f.capabilities.nrf && !f.capabilities.cc && !f.capabilities.esp_wifi_verified);
    f.capabilities.nrf = f.capabilities.cc = true; /* transport must ignore erroneous capabilities */
    assert(f.platform.acquire(&f, NS_RADIO_NRF) == NS_UNSUPPORTED);
    assert(f.platform.acquire(&f, NS_RADIO_CC) == NS_UNSUPPORTED);
    f.capabilities.nrf = f.capabilities.cc = false;
    mock_serial_pin_mismatch(true);
    assert(f.platform.acquire(&f, NS_RADIO_ESP) == NS_UNSUPPORTED && !f.serial);
    mock_serial_pin_mismatch(false);
    assert(f.platform.acquire(&f, NS_RADIO_ESP) == NS_OK);
    const char* denied[] = {"help\n","stopscan\n","sniffbeacon\n","info -a 0\n","reboot\n","info\nhelp\n"};
    f.capabilities.esp_wifi_verified = true; /* transport gate must still prevail */
    for(size_t i=0; i<sizeof(denied)/sizeof(denied[0]); i++)
        assert(f.platform.uart_write(&f, (const uint8_t*)denied[i], strlen(denied[i])) == NS_UNSUPPORTED);
    assert(f.platform.uart_write(&f, (const uint8_t*)"info\n", 5) == NS_OK);
    assert(f.platform.uart_write(&f, (const uint8_t*)"info\n", 5) == NS_BUSY);
    assert(!strcmp(mock_serial_commands(), "info\n"));
    f.platform.release(&f, NS_RADIO_ESP); f.capabilities.esp_wifi_verified = false;
    NsApplication a; assert(ns_app_init(&a, &f.platform, &f.store, f.capabilities) == NS_ABSENT);
    a.page = NS_PAGE_RADIO; a.selected = NS_RADIO_ESP;
    check_frame(&a, "Disconnected: USART closed");
    assert(ns_app_key(&a, NS_KEY_OK, 0) == NS_OK && !f.serial);
    check_frame(&a, "Connecting: USART requested");
    assert(ns_app_step(&a, 1) == NS_OK && !f.serial);
    assert(ns_app_step(&a, 2) == NS_OK && f.serial);
    check_frame(&a, "Waiting: info response");
    const char* private_line = "Serial Number: PRIVATE_TEST_ID\n";
    mock_serial_feed((const uint8_t*)private_line, strlen(private_line));
    char reply[1024]; fixture(reply); mock_serial_feed((const uint8_t*)reply, strlen(reply));
    assert(ns_app_step(&a, 3) == NS_OK && !f.serial && f.owner == NS_RADIO_NONE);
    check_frame(&a, "Connected: query passed");
    assert(a.session.marauder.identity_matches && a.saved.history_count == 1);
    for(size_t i=0; i<sizeof(f.rx); i++) assert(f.rx[i] == 0);
    assert(!atomic_load(&f.rx_head) && !atomic_load(&f.rx_tail) && !f.rx_total && !f.info_sent);
    /* History snapshots and closed CSV contain no arbitrary serial identifiers. */
    uint8_t persisted[NS_STATE_CAPACITY]; size_t length = 0;
    for(unsigned slot=0; slot<2; slot++) {
        assert(f.store.read(f.store.ctx, slot, persisted, sizeof(persisted), &length) == NS_OK);
        for(size_t j=0; j+strlen("PRIVATE_TEST_ID")<=length; j++)
            assert(memcmp(persisted+j, "PRIVATE_TEST_ID", strlen("PRIVATE_TEST_ID")));
    }
    File* csv = storage_file_alloc(f.storage); assert(csv);
    assert(storage_file_open(csv, "/ext/apps_data/netsentinel/session_0000000001.csv", FSAM_READ, FSOM_OPEN_EXISTING));
    uint8_t telemetry[2048]; size_t telemetry_length = storage_file_read(csv, telemetry, sizeof(telemetry));
    assert(telemetry_length == storage_file_size(csv));
    for(size_t j=0; j+strlen("PRIVATE_TEST_ID")<=telemetry_length; j++)
        assert(memcmp(telemetry+j, "PRIVATE_TEST_ID", strlen("PRIVATE_TEST_ID")));
    assert(storage_file_close(csv)); storage_file_free(csv);
    size_t stale_count = 99; uint8_t stale;
    assert(f.platform.uart_read(&f, &stale, 1, &stale_count) == NS_INVALID && !stale_count);
    f.platform.release(&f, NS_RADIO_ESP); /* already released: no double HAL cleanup */
    a.session.marauder.identity_matches = false;
    strcpy(a.session.marauder.hardware, "PRIVATE_TEST_ID");
    NsFrame redacted; ns_app_frame(&a, &redacted);
    for(unsigned row=0; row<6; row++) assert(!strstr(redacted.rows[row], "PRIVATE_TEST"));
    a.status=NS_INVALID; check_frame(&a, "Error: reply/state invalid");
    a.status=NS_IO; check_frame(&a, "Error: serial or SD I/O");
    a.status=NS_FULL; check_frame(&a, "Error: response/storage full");
    a.status=NS_UNSUPPORTED; check_frame(&a, "Error: USART configuration");
    a.status=NS_OK;
    assert(!strcmp(mock_serial_commands(), "info\n"));
    assert(ns_app_key(&a, NS_KEY_OK, 4) == NS_OK);
    assert(ns_app_key(&a, NS_KEY_BACK, 5) == NS_OK && !f.serial && !a.info_pending);
    a.page = NS_PAGE_RADIO;
    assert(ns_app_device_info(&a, 10) == NS_OK);
    assert(ns_app_step(&a, 5010) == NS_TIMEOUT && !f.serial);
    check_frame(&a, "Timeout: link unconfirmed");
    FuriHalSerialHandle* other = furi_hal_serial_control_acquire(FuriHalSerialIdUsart);
    assert(other && ns_app_device_info(&a, 5011) == NS_BUSY);
    check_frame(&a, "Error: USART/session busy");
    furi_hal_serial_control_release(other);
    assert(ns_app_device_info(&a, 5012) == NS_OK);
    assert(ns_app_stop(&a, 5013) == NS_OK && !f.serial);
    assert(ns_app_device_info(&a, 5014) == NS_OK);
    /* Overall reply budget also applies when the worker drains the ring. */
    uint8_t bytes[128]; memset(bytes, 'x', sizeof(bytes)); size_t n;
    for(unsigned i=0; i<NS_UART_CAPACITY/sizeof(bytes); i++) {
        mock_serial_feed(bytes, sizeof(bytes));
        assert(f.platform.uart_read(&f, bytes, sizeof(bytes), &n) == NS_OK && n == sizeof(bytes));
    }
    mock_serial_feed(bytes, 1);
    assert(ns_app_step(&a, 5015) == NS_FULL && !f.serial);
    assert(ns_app_start(&a, NS_RADIO_NRF, false, true, 5016) == NS_UNSUPPORTED);
    assert(ns_app_start(&a, NS_RADIO_CC, false, true, 5016) == NS_UNSUPPORTED);
    ns_flipper_deinit(&f); assert(!mock_sdk_live() && !mock_sdk_records());
    repeated();
    puts("PASS: info-only USART mapping/framing, command exclusion, state UI, response budget and cleanup (MOCK)");
    return 0;
}
