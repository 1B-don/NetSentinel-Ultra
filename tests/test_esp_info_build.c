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
static void diagnostic_state_cases(void) {
    mock_sdk_reset(); NsFlipper f; NsApplication a;
    assert(ns_flipper_init(&f));
    assert(ns_app_init(&a,&f.platform,&f.store,f.capabilities)==NS_ABSENT);
    a.page=NS_PAGE_RADIO; a.selected=NS_RADIO_ESP;
    assert(a.info_state==NS_ESP_DISCONNECTED && !a.info_attempted);
    char reply[1024]; fixture(reply);
    assert(ns_app_device_info(&a,0)==NS_OK && a.info_state==NS_ESP_CONNECTING);
    /* Fields/prompt without echo cannot complete a transaction. */
    const char* stale=strchr(reply,'\n')+1;
    mock_serial_feed((const uint8_t*)stale,strlen(stale));
    assert(ns_app_step(&a,1)==NS_OK && f.serial && !a.info_validated);
    for(size_t i=0;i<strlen(reply);i++) {
        mock_serial_feed((const uint8_t*)reply+i,1);
        assert(ns_app_step(&a,(uint32_t)i+2)==NS_OK);
    }
    assert(a.info_state==NS_ESP_CONNECTED && a.info_reply_complete && !f.serial);
    a.info_view=3; NsFrame frame; ns_app_frame(&a,&frame);
    assert(!strcmp(frame.rows[2],"Complete framed reply") && !strcmp(frame.rows[3],"Result: OK"));
    assert(ns_app_key(&a,NS_KEY_DOWN,500)==NS_OK && a.info_view==0);
    assert(ns_app_key(&a,NS_KEY_UP,500)==NS_OK && a.info_view==3);
    a.status=NS_IO; check_frame(&a,"Connected: query passed");
    assert(ns_app_device_info(&a,600)==NS_OK);
    const char* duplicate="#info\nFirmware: Marauder\n#info\n";
    mock_serial_feed((const uint8_t*)duplicate,strlen(duplicate));
    assert(ns_app_step(&a,601)==NS_INVALID && !f.serial);
    assert(a.info_state==NS_ESP_PROTOCOL_ERROR && a.info_result==NS_INVALID);
    ns_app_frame(&a,&frame);
    assert(!strcmp(frame.rows[2],"No complete framed reply") && !strcmp(frame.rows[3],"Result: Invalid"));
    assert(a.info_validated && !a.info_reply_complete); /* Historical identity survives. */
    assert(ns_app_device_info(&a,700)==NS_OK);
    assert(ns_app_step(&a,5700)==NS_TIMEOUT && a.info_state==NS_ESP_TIMEOUT && !f.serial);
    assert(ns_app_device_info(&a,5800)==NS_OK);
    assert(ns_app_stop(&a,5801)==NS_OK && a.info_state==NS_ESP_CANCELLED && a.info_result==NS_CANCELLED);
    assert(!f.serial && !f.owner);
    assert(ns_app_device_info(&a,5900)==NS_OK);
    mock_sdk_fail_storage(true);
    assert(ns_app_stop(&a,5901)==NS_IO && !f.serial && !f.owner);
    assert(a.info_state==NS_ESP_CANCELLED && a.info_result==NS_IO);
    mock_sdk_fail_storage(false);
    assert(ns_app_stop(&a,5902)==NS_OK && !a.history_dirty);
    assert(a.info_result==NS_IO); /* Recovery must not hide the failed cleanup. */
    /* Bytes buffered before dispatch cannot act as its reply. */
    assert(f.platform.acquire(&f,NS_RADIO_ESP)==NS_OK);
    mock_serial_feed((const uint8_t*)reply,strlen(reply));
    assert(f.platform.uart_write(&f,(const uint8_t*)"info\n",5)==NS_OK);
    uint8_t bytes[16];size_t count=99;
    assert(f.platform.uart_read(&f,bytes,sizeof(bytes),&count)==NS_OK && !count);
    f.platform.release(&f,NS_RADIO_ESP);
    ns_flipper_deinit(&f); assert(!mock_sdk_live() && !mock_sdk_records());
}
static void repeated(void) {
    mock_sdk_reset(); NsFlipper f; NsApplication a; assert(ns_flipper_init(&f));
    assert(ns_app_init(&a,&f.platform,&f.store,f.capabilities)==NS_ABSENT);
    a.page=NS_PAGE_RADIO; a.selected=NS_RADIO_ESP;
    assert(ns_app_key(&a,NS_KEY_OK,0)==NS_OK);
    assert(ns_app_device_info(&a,0)==NS_BUSY && !f.serial && a.info_pending==2);
    assert(ns_app_stop(&a,0)==NS_OK && !a.info_pending);
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
    const char* denied[] = {"help\n","stopscan\n","sniffbeacon\n","info -a 0\n","reboot\n","info\nhelp\n","scanap\n","deauth\n","inject\n","reset\n","update\n","flash\n"};
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
    assert(ns_app_key(&a,NS_KEY_OK,0)==NS_OK && !f.serial);
    assert(ns_app_key(&a,NS_KEY_RIGHT,0)==NS_BUSY && !f.serial);
    assert(ns_app_key(&a,NS_KEY_BACK,0)==NS_OK && !a.info_pending && !f.serial && !a.saved.last_id);
    a.page=NS_PAGE_RADIO; check_frame(&a,"Cancelled: USART closed");
    assert(ns_app_key(&a, NS_KEY_OK, 0) == NS_OK && !f.serial);
    check_frame(&a, "Connecting: USART requested");
    assert(ns_app_step(&a, 1) == NS_OK && !f.serial);
    assert(ns_app_step(&a, 2) == NS_OK && f.serial);
    check_frame(&a, "Connecting: waiting for info");
    const char* private_line = "Serial Number: PRIVATE_TEST_ID\n";
    mock_serial_feed((const uint8_t*)private_line, strlen(private_line));
    char reply[1024]; fixture(reply); mock_serial_feed((const uint8_t*)reply, strlen(reply));
    assert(ns_app_step(&a, 3) == NS_OK && !f.serial && f.owner == NS_RADIO_NONE);
    check_frame(&a, "Connected: query passed");
    assert(a.session.marauder.identity_matches && a.saved.history_count == 1);
    assert(a.info_success_id == 1 && a.info_elapsed_ms == 1 && a.info_validated);
    NsFrame details;
    assert(ns_app_key(&a,NS_KEY_DOWN,3)==NS_OK); ns_app_frame(&a,&details);
    assert(!strcmp(details.rows[2],"v5.5.1-710-g8410210c9a"));
    assert(strstr(details.rows[3],"Connected"));
    assert(ns_app_key(&a,NS_KEY_DOWN,3)==NS_OK); ns_app_frame(&a,&details);
    assert(strstr(details.rows[1],"Last success: 1") && strstr(details.rows[2],"1 ms"));
    a.info_view=0;
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
    /* Unrelated UI/storage outcomes cannot overwrite the last info result. */
    a.status=NS_INVALID; check_frame(&a, "Connected: query passed");
    a.status=NS_IO; check_frame(&a, "Connected: query passed");
    a.status=NS_FULL; check_frame(&a, "Connected: query passed");
    a.status=NS_UNSUPPORTED; check_frame(&a, "Connected: query passed");
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
    check_frame(&a, "Disconnected: USART busy");
    furi_hal_serial_control_release(other);
    assert(ns_app_device_info(&a, 5012) == NS_OK);
    const char* fragment="#info\nVersion: v1.";
    mock_serial_feed((const uint8_t*)fragment,strlen(fragment));
    assert(ns_app_step(&a,5012)==NS_OK);
    assert(ns_app_key(&a,NS_KEY_OK,5012)==NS_BUSY && f.serial);
    assert(ns_app_key(&a,NS_KEY_BACK,5013)==NS_OK && !f.serial && !a.info_pending);
    a.page=NS_PAGE_RADIO; check_frame(&a,"Cancelled: USART closed");
    assert(a.info_success_id==1 && a.info_elapsed_ms==1);
    size_t late_count=99; uint8_t late_byte;
    assert(f.platform.uart_read(&f,&late_byte,1,&late_count)==NS_INVALID && !late_count);
    assert(ns_app_step(&a,5013)==NS_OK && !f.serial);
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
    assert(ns_app_device_info(&a,5020)==NS_OK);
    const char* disconnected="#info\nFirmware: Marauder\nVersion: v1.11.0\nHardware: Marauder v4\nESP-IDF: v5.5.1-710-g8410210c9a\nWSL Bypass: disabled\nSD Card: Not Connected\nSD Card Size: 0\nBattery Monitor: not supported\n> ";
    mock_serial_feed((const uint8_t*)disconnected,strlen(disconnected));
    assert(ns_app_step(&a,5021)==NS_OK && !f.serial && a.info_validated && !a.info_sd_connected);
    a.page=NS_PAGE_RADIO;a.info_view=1;NsFrame no_sd;ns_app_frame(&a,&no_sd);
    assert(strstr(no_sd.rows[3],"Disconnected") && !strstr(no_sd.rows[3],"PRIVATE"));
    check_frame(&a,"Connected: query passed");
    /* An incomplete or mismatched framed reply must not be an OK history
     * entry, even when the previous query had a valid historical snapshot. */
    assert(ns_app_device_info(&a,5022)==NS_OK);
    const char* partial="#info\nVersion: v1.11.0\nHardware: Marauder v4\nESP-IDF: v5.5.1-710-g8410210c9a\n> ";
    mock_serial_feed((const uint8_t*)partial,strlen(partial));
    assert(ns_app_step(&a,5023)==NS_INVALID && !f.serial && a.info_validated);
    assert(a.saved.history[a.saved.history_count-1].result==NS_INVALID);
    assert(a.info_success_id==a.saved.history[a.saved.history_count-2].id);
    assert(ns_app_device_info(&a,5024)==NS_OK);
    char mismatch[1024]; strcpy(mismatch,disconnected);
    char* version=strstr(mismatch,"v1.11.0"); assert(version); version[6]='1';
    mock_serial_feed((const uint8_t*)mismatch,strlen(mismatch));
    assert(ns_app_step(&a,5025)==NS_UNSUPPORTED && !f.serial);
    assert(a.saved.history[a.saved.history_count-1].result==NS_UNSUPPORTED);
    assert(!a.session.marauder.identity_matches && a.info_validated);
    for(unsigned bad=0;bad<3;bad++) {
        assert(ns_app_device_info(&a,5030+bad*2)==NS_OK);
        uint8_t malformed[]={'#','i','n','f','o','\n','X',0,'\n'};
        malformed[7]=bad==0 ? 0 : (bad==1 ? 1 : 255);
        mock_serial_feed(malformed,sizeof(malformed));
        assert(ns_app_step(&a,5031+bad*2)==NS_INVALID && !f.serial);
        assert(a.saved.history[a.saved.history_count-1].result==NS_INVALID);
        assert(!strcmp(mock_serial_commands(),"info\n"));
    }
    assert(ns_app_device_info(&a,5040)==NS_OK);
    const char* embedded_cr="#info\nVersion: v1.\r";
    mock_serial_feed((const uint8_t*)embedded_cr,strlen(embedded_cr));
    assert(ns_app_step(&a,5041)==NS_OK && f.serial);
    const char* repaired="11.0\n";
    mock_serial_feed((const uint8_t*)repaired,strlen(repaired));
    assert(ns_app_step(&a,5042)==NS_INVALID && !f.serial && !a.session.pending_cr);
    assert(a.saved.history[a.saved.history_count-1].result==NS_INVALID);
    ns_flipper_deinit(&f); assert(!mock_sdk_live() && !mock_sdk_records());
    repeated();
    diagnostic_state_cases();
    puts("PASS: info-only USART mapping/framing, command exclusion, state UI, response budget and cleanup (MOCK)");
    return 0;
}
