#include "netsentinel/application.h"
#include "mock_platform.h"
#include <stdio.h>
typedef struct {
    uint8_t data[2][NS_STATE_CAPACITY]; size_t sizes[2];
    bool exists[2], fail_read, fail_write, corrupt_readback;
    size_t cut;
    unsigned writes;
} StoreMock;
static NsResult read_slot(void* ctx, unsigned slot, uint8_t* data, size_t cap, size_t* n) {
    StoreMock* m = ctx; assert(slot < 2);
    if(m->fail_read) return NS_IO;
    if(!m->exists[slot]) return NS_ABSENT;
    *n = m->sizes[slot]; if(*n > cap) return NS_FULL;
    memcpy(data, m->data[slot], *n);
    if(m->corrupt_readback && *n) data[*n-1] ^= 1;
    return NS_OK;
}
static NsResult write_slot(void* ctx, unsigned slot, const uint8_t* data, size_t n) {
    StoreMock* m = ctx; assert(slot < 2 && n <= NS_STATE_CAPACITY); m->writes++;
    m->exists[slot] = true;
    size_t written = m->fail_write ? (m->cut < n ? m->cut : n) : n;
    memcpy(m->data[slot], data, written); m->sizes[slot] = written;
    return m->fail_write ? NS_IO : NS_OK;
}
static NsStore store(StoreMock* m) { return (NsStore){m, read_slot, write_slot}; }
static void test_persistence(void) {
    StoreMock m = {0}; NsStore io = store(&m); NsPersistence p; NsSavedState s;
    assert(ns_persistence_load(&p, &io, &s) == NS_ABSENT);
    s.settings.timeout_ms = 12000; s.last_id = 100;
    assert(ns_persistence_save(&p, &s) == NS_OK && p.generation == 1);
    s.history_count = NS_HISTORY_LIMIT;
    for(unsigned i = 0; i < NS_HISTORY_LIMIT; i++) s.history[i] = (NsHistory){i+1, UINT32_MAX-10, 20, 5, NS_RADIO_NRF, NS_OK};
    assert(ns_persistence_save(&p, &s) == NS_OK && p.generation == 2);
    NsPersistence reboot; NsSavedState recovered;
    assert(ns_persistence_load(&reboot, &io, &recovered) == NS_OK);
    assert(recovered.history_count == NS_HISTORY_LIMIT && recovered.history[15].id == 16);
    m.data[p.slot][5] ^= 1;
    assert(ns_persistence_load(&reboot, &io, &recovered) == NS_OK && reboot.generation == 1);
    assert(recovered.last_id == 100 && !recovered.history_count);
    m.data[0][0] = 0;
    assert(ns_persistence_load(&reboot, &io, &recovered) == NS_INVALID && !reboot.loaded);
    assert(ns_persistence_save(&reboot, &s) == NS_INVALID);
    m.fail_read = true;
    assert(ns_persistence_load(&reboot, &io, &recovered) == NS_IO);
}
static void test_record_word(uint8_t* p, uint32_t value) {
    for(unsigned i = 0; i < 4; i++) p[i] = (uint8_t)(value >> (8*i));
}
static void test_record_crc(uint8_t* data, size_t n) {
    uint32_t crc = UINT32_MAX;
    for(size_t i = 0; i < n-4; i++) {
        crc ^= data[i];
        for(unsigned j = 0; j < 8; j++) crc = (crc >> 1) ^ (0xedb88320u & (0u-(crc & 1u)));
    }
    test_record_word(data+n-4, ~crc);
}
static void test_invalid_history_enums(void) {
    StoreMock disk = {0}; NsStore io = store(&disk);
    NsPersistence p; NsSavedState s;
    assert(ns_persistence_load(&p, &io, &s) == NS_ABSENT);
    s.last_id = 1; assert(ns_persistence_save(&p, &s) == NS_OK);
    unsigned older = p.slot;
    s.history_count = 1; s.history[0] = (NsHistory){1, 10, 20, 1, NS_RADIO_NRF, NS_OK};
    assert(ns_persistence_save(&p, &s) == NS_OK);
    StoreMock valid_disk = disk;
    const uint32_t bad_results[] = {NS_FULL+1, 255, 256, UINT32_MAX};
    const uint32_t bad_radios[] = {NS_RADIO_NONE, NS_RADIO_CC+1, 255, 256, 257, UINT32_MAX};
    for(unsigned field = 0; field < 2; field++) {
        const uint32_t* values = field ? bad_radios : bad_results;
        size_t count = field ? sizeof(bad_radios)/sizeof(*bad_radios) : sizeof(bad_results)/sizeof(*bad_results);
        for(size_t i = 0; i < count; i++) {
            disk = valid_disk;
            test_record_word(disk.data[p.slot]+32+(field ? 16 : 20), values[i]);
            test_record_crc(disk.data[p.slot], disk.sizes[p.slot]);
            /* CRC is valid: range validation alone must reject this snapshot. */
            NsPersistence boot; NsSavedState recovered;
            assert(ns_persistence_load(&boot, &io, &recovered) == NS_OK);
            assert(boot.slot == older && boot.generation == 1 && !recovered.history_count);
            disk.exists[older] = false; recovered.last_id = 123456;
            unsigned writes = disk.writes;
            assert(ns_persistence_load(&boot, &io, &recovered) == NS_INVALID);
            assert(!boot.loaded && recovered.last_id == 123456);
            assert(ns_persistence_save(&boot, &s) == NS_INVALID && disk.writes == writes);
        }
    }
    disk = valid_disk;
    unsigned writes = disk.writes; uint32_t generation = p.generation;
    s.history[0].result = (NsResult)-1;
    assert(ns_persistence_save(&p, &s) == NS_INVALID);
    s.history[0].result = (NsResult)(NS_FULL+1);
    assert(ns_persistence_save(&p, &s) == NS_INVALID);
    assert(disk.writes == writes && p.generation == generation);
}
static void test_torn_writes(void) {
    /* Every byte boundary in a full history snapshot. Older slot must survive. */
    for(size_t cut = 0; cut < 420; cut++) {
        StoreMock m = {0}; NsStore io = store(&m); NsPersistence p; NsSavedState s;
        assert(ns_persistence_load(&p, &io, &s) == NS_ABSENT);
        s.last_id = 16;
        assert(ns_persistence_save(&p, &s) == NS_OK);
        s.history_count = 16;
        for(unsigned i = 0; i < 16; i++) s.history[i] = (NsHistory){i+1, 10, 20, 2, NS_RADIO_CC, NS_OK};
        m.fail_write = true; m.cut = cut;
        assert(ns_persistence_save(&p, &s) == NS_IO && p.generation == 1);
        NsPersistence boot; NsSavedState recovered;
        assert(ns_persistence_load(&boot, &io, &recovered) == NS_OK);
        assert(boot.generation == 1 && !recovered.history_count && recovered.last_id == 16);
    }
}
static void test_app(void) {
    Mock m = {.rssi = -70}; NsPlatform radio = platform(&m);
    StoreMock disk = {0}; NsStore io = store(&disk); NsApplication app;
    NsCapabilities caps = {.nrf=true, .cc=true};
    assert(ns_app_init(&app, &radio, &io, caps) == NS_ABSENT);
    assert(ns_app_start(&app, NS_RADIO_ESP, false, false, 0) == NS_UNSUPPORTED);
    assert(ns_app_start(&app, NS_RADIO_ESP, true, false, 0) == NS_UNSUPPORTED);
    assert(!m.acquired && !m.uart_calls && !disk.writes);
    NsSettings settings = app.saved.settings; settings.dwell_us = 250; settings.timeout_ms = 11000;
    assert(ns_app_save_settings(&app, &settings) == NS_OK);
    assert(ns_app_init(&app, &radio, &io, caps) == NS_OK && app.session.dwell_us == 250);
    NsFrame frame;
    assert(ns_app_key(&app, NS_KEY_DOWN, 1) == NS_OK);
    assert(ns_app_key(&app, NS_KEY_DOWN, 1) == NS_OK);
    assert(ns_app_key(&app, NS_KEY_OK, 1) == NS_OK && app.page == NS_PAGE_RADIO);
    assert(ns_app_key(&app, NS_KEY_OK, 1) == NS_OK && app.page == NS_PAGE_SELECTOR && !m.acquired);
    ns_app_frame(&app, &frame); assert(strstr(frame.rows[1], "NRF24"));
    assert(ns_app_key(&app, NS_KEY_BACK, 1) == NS_OK && !m.acquired);
    assert(ns_app_key(&app, NS_KEY_OK, 1) == NS_OK);
    assert(ns_app_key(&app, NS_KEY_OK, 1) == NS_OK);
    assert(ns_app_key(&app, NS_KEY_OK, 2) == NS_OK && app.session.active == NS_RADIO_NRF);
    assert(app.saved.last_id == 1 && app.session.logging);
    assert(ns_app_save_settings(&app, &settings) == NS_BUSY);
    m.regs[9] = 1;
    for(unsigned ch = 0; ch < 126; ch++) assert(ns_app_step(&app, 3+ch) == NS_OK);
    assert(m.delay == 5000+126*250);
    ns_app_frame(&app, &frame); assert(frame.bar_count == 126 && frame.bars[125] == 22);
    assert(strstr(m.file, "nrf,,125,100,percent"));
    assert(ns_app_key(&app, NS_KEY_BACK, 130) == NS_OK && !m.owner && !app.session.logging);
    assert(app.saved.history_count == 1 && app.saved.history[0].observations == 126);
    assert(ns_app_init(&app, &radio, &io, caps) == NS_OK && app.session.id == 1);
    assert(app.saved.history_count == 1);
    assert(ns_app_start(&app, NS_RADIO_CC, false, false, 200) == NS_INVALID);
    assert(ns_app_start(&app, NS_RADIO_CC, false, true, 200) == NS_OK && app.session.id == 2);
    assert(m.frequency == settings.cc_frequency_hz);
    assert(ns_app_step(&app, 201) == NS_OK);
    disk.fail_write = true;
    assert(ns_app_stop(&app, 202) == NS_IO && !m.owner && app.history_dirty);
    unsigned acquisitions = m.acquired;
    assert(ns_app_start(&app, NS_RADIO_NRF, false, true, 203) == NS_IO && m.acquired == acquisitions);
    disk.fail_write = false;
    assert(ns_app_stop(&app, 204) == NS_OK && !app.history_dirty);
    assert(ns_app_init(&app, &radio, &io, caps) == NS_OK && app.saved.history_count == 2);
    settings.timeout_ms = 22000; disk.fail_write = true;
    assert(ns_app_save_settings(&app, &settings) == NS_IO && app.saved.settings.timeout_ms == 11000);
    disk.fail_write = false;
    m.fail_write = true;
    assert(ns_app_start(&app, NS_RADIO_NRF, false, true, 300) == NS_IO);
    assert(!m.owner && !app.session.logging && app.saved.last_id == 3);
    m.fail_write = false;
    assert(ns_app_init(&app, &radio, &io, caps) == NS_OK && app.session.id == 3);
    assert(ns_app_start(&app, NS_RADIO_CC, false, true, 400) == NS_OK && app.session.id == 4);
    assert(ns_app_stop(&app, 401) == NS_OK);
    app.page = NS_PAGE_HOME;
    assert(ns_app_key(&app, NS_KEY_BACK, 402) == NS_OK && !app.running);
    assert(m.acquired == m.released && !m.uart_calls);
}
static void test_history_wrap_and_exhaustion(void) {
    StoreMock disk = {0}; NsStore io = store(&disk);
    Mock m = {0}; NsPlatform radio = platform(&m); NsApplication a;
    assert(ns_app_init(&a, &radio, &io, (NsCapabilities){.nrf=true, .cc=true}) == NS_ABSENT);
    for(unsigned i = 0; i < 24; i++) {
        assert(ns_app_start(&a, NS_RADIO_CC, false, true, i*100) == NS_OK);
        assert(ns_app_step(&a, i*100+1) == NS_OK);
        assert(ns_app_stop(&a, i*100+2) == NS_OK);
    }
    assert(ns_app_init(&a, &radio, &io, (NsCapabilities){.nrf=true, .cc=true}) == NS_OK);
    assert(a.saved.history_count == 16 && a.saved.history[0].id == 9 && a.saved.history[15].id == 24);
    a.page = NS_PAGE_HISTORY; NsFrame frame; ns_app_frame(&a, &frame);
    assert(strstr(frame.rows[1], "Session 24"));
    assert(ns_app_key(&a, NS_KEY_DOWN, 3000) == NS_OK); ns_app_frame(&a, &frame);
    assert(strstr(frame.rows[1], "Session 23"));
    a.saved.last_id = UINT32_MAX;
    unsigned acquired = m.acquired;
    assert(ns_app_start(&a, NS_RADIO_NRF, false, true, 3000) == NS_FULL && m.acquired == acquired);
    a.persistence.generation = UINT32_MAX;
    NsSettings settings = a.saved.settings;
    assert(ns_app_save_settings(&a, &settings) == NS_OK); /* unchanged needs no generation */
    settings.timeout_ms++;
    assert(ns_app_save_settings(&a, &settings) == NS_FULL);
}
static void test_recovery_and_cleanup(void) {
    StoreMock disk = {.fail_read = true}; NsStore io = store(&disk);
    Mock m = {0}; NsPlatform radio = platform(&m); NsApplication a;
    assert(ns_app_init(&a, &radio, &io, (NsCapabilities){.nrf=true, .cc=true}) == NS_IO);
    assert(ns_app_start(&a, NS_RADIO_NRF, false, true, 0) == NS_INVALID && !m.acquired);
    disk.fail_read = false;
    assert(ns_app_reload(&a) == NS_OK);
    assert(ns_app_start(&a, NS_RADIO_CC, false, true, 10) == NS_OK);
    assert(ns_app_reload(&a) == NS_BUSY);
    m.fail_idle = true;
    assert(ns_app_stop(&a, 11) == NS_IO && !m.owner);
    assert(a.saved.history[0].result == NS_IO);
    m.fail_idle = false;
    assert(ns_app_start(&a, NS_RADIO_NRF, false, true, 20) == NS_OK);
    m.fail_spi = true;
    assert(ns_app_stop(&a, 21) == NS_IO && !m.owner && !m.ce);
    assert(a.saved.history[1].result == NS_IO);
    m.fail_spi = false;
    assert(ns_app_start(&a, NS_RADIO_CC, false, true, 30) == NS_OK);
    m.fail_close = true;
    assert(ns_app_stop(&a, 31) == NS_IO && !a.session.logging && !m.owner);
    assert(a.saved.history[2].result == NS_IO);
}
static void test_readback_and_disabled(void) {
    StoreMock disk = {0}; NsStore io = store(&disk); Mock m = {0}; NsPlatform radio = platform(&m); NsApplication a;
    assert(ns_app_init(&a, &radio, &io, (NsCapabilities){0}) == NS_ABSENT);
    assert(ns_app_start(&a, NS_RADIO_NRF, false, true, 0) == NS_UNSUPPORTED && !disk.writes);
    assert(ns_app_init(&a, &radio, &io, (NsCapabilities){.nrf=true, .cc=true}) == NS_ABSENT);
    disk.corrupt_readback = true;
    assert(ns_app_start(&a, NS_RADIO_NRF, false, true, 0) == NS_IO && !m.acquired);
    disk.corrupt_readback = false;
    assert(ns_app_init(&a, &radio, &io, (NsCapabilities){.nrf=true, .cc=true}) == NS_OK && a.saved.last_id == 1);
    assert(ns_app_start(&a, NS_RADIO_NRF, false, true, 0) == NS_OK && a.session.id == 2);
    assert(ns_app_stop(&a, 1) == NS_OK);
}
static NsResult cc_diagnostic(void* ctx, NsCcDiagnostics* out) {
    Mock* m = ctx; assert(m->owner == NS_RADIO_CC);
    if(m->fail_spi) return NS_TIMEOUT;
    *out = (NsCcDiagnostics){.part=0, .version=0x14, .state=m->frequency ? 13 : 1, .transactions=7};
    return NS_OK;
}
static void test_cc_explorer(void) {
    StoreMock disk = {0}; NsStore io = store(&disk); Mock m = {.rssi=-120};
    NsPlatform radio = platform(&m); radio.cc_diagnose = cc_diagnostic; NsApplication a;
    NsCapabilities caps = {.nrf=true, .cc=true, .esp_uart=true};
    assert(ns_app_init(&a, &radio, &io, caps) == NS_ABSENT);
    a.selected = NS_RADIO_CC; a.page = NS_PAGE_RADIO;
    assert(ns_app_key(&a, NS_KEY_UP, 0) == NS_OK && a.saved.settings.cc_frequency_hz == 868000000 && !m.acquired);
    assert(ns_app_init(&a, &radio, &io, caps) == NS_OK && a.saved.settings.cc_frequency_hz == 868000000);
    a.selected = NS_RADIO_CC; a.page = NS_PAGE_RADIO;
    assert(ns_app_key(&a, NS_KEY_OK, 0) == NS_OK && a.page == NS_PAGE_SELECTOR);
    assert(ns_app_key(&a, NS_KEY_OK, 1) == NS_OK && m.frequency == 868000000);
    assert(ns_app_step(&a, 2) == NS_OK); m.rssi = -70;
    assert(ns_app_step(&a, 102) == NS_OK); m.rssi = -20;
    assert(ns_app_step(&a, 202) == NS_OK);
    NsFrame f; ns_app_frame(&a, &f);
    assert(f.cc_plot && f.bar_count == 3 && f.bars[0] == 0 && f.bars[1] == 5 && f.bars[2] == 10);
    assert(strstr(f.rows[2], "Mean -70") && strstr(f.rows[4], "66%"));
    assert(strstr(m.file, "cc,,868000000,-20,dBm") && strstr(m.file, "cc_activity,above_-80,868000000,66,percent"));
    assert(ns_app_key(&a, NS_KEY_DOWN, 203) == NS_OK); ns_app_frame(&a, &f);
    assert(!f.cc_plot && strstr(f.rows[2], "ID 00/14") && strstr(m.file, "cc_diag,version"));
    assert(ns_app_key(&a, NS_KEY_UP, 204) == NS_OK && m.frequency == 915000000 && !a.session.cc.samples);
    assert(a.saved.settings.cc_frequency_hz == 868000000); /* active exploration is temporary */
    assert(ns_app_step(&a, 204) == NS_OK && a.session.observations == 4);
    assert(ns_app_cc_tune(&a, 350000000, 205) == NS_INVALID && m.frequency == 915000000);
    m.fail_spi = true;
    assert(ns_app_step(&a, 304) == NS_TIMEOUT && !m.owner && !a.history_dirty);
    assert(a.saved.history_count == 1 && a.saved.history[0].result == NS_TIMEOUT);
    m.fail_spi = false;
    assert(ns_app_start(&a, NS_RADIO_NRF, false, true, 305) == NS_OK);
    assert(ns_app_cc_tune(&a, 433920000, 306) == NS_BUSY && m.owner == NS_RADIO_NRF);
    assert(ns_app_stop(&a, 307) == NS_OK);
    assert(ns_app_start(&a, NS_RADIO_ESP, false, false, 308) == NS_UNSUPPORTED && !m.uart_calls);
    assert(ns_app_cc_tune(&a, 433920000, 309) == NS_OK && !a.session.cc.samples);
    assert(ns_app_init(&a, &radio, &io, caps) == NS_OK && a.saved.settings.cc_frequency_hz == 433920000);
    assert(ns_app_start(&a, NS_RADIO_CC, false, true, 310) == NS_OK);
    assert(ns_app_step(&a, 311) == NS_OK && a.session.cc.samples == 1);
    assert(ns_app_stop(&a, 312) == NS_OK && m.acquired == m.released);
    a.page = NS_PAGE_SETTINGS; a.setting = 2;
    disk.fail_write = true;
    assert(ns_app_key(&a, NS_KEY_RIGHT, 313) == NS_IO && a.session.cc.samples == 1);
    disk.fail_write = false;
    assert(ns_app_key(&a, NS_KEY_RIGHT, 314) == NS_OK && !a.session.cc.samples && !a.session.cc.diagnostic_valid);
}
static void test_exports(void) {
    Mock m={0}; StoreMock disk={0}; NsPlatform radio=platform(&m); NsStore io=store(&disk);
    NsApplication a; assert(ns_app_init(&a,&radio,&io,(NsCapabilities){0})==NS_ABSENT);
    a.saved.last_id=3; a.saved.history_count=2;
    a.saved.history[0]=(NsHistory){1,UINT32_MAX-10,20,10,NS_RADIO_ESP,NS_OK};
    a.saved.history[1]=(NsHistory){3,10,20,4,NS_RADIO_CC,NS_IO};
    a.history_filter.radio=NS_RADIO_CC;
    assert(ns_app_export_history(&a,100)==NS_OK && a.export_id==4 && a.saved.last_id==4);
    assert(strstr(m.file,"history_result,radio_3,3,4,enum") && !strstr(m.file,"radio_1"));
    assert(m.acquired==0 && m.uart_calls==0 && m.closes==1);
    a.info_pending=2;assert(ns_app_export_history(&a,101)==NS_BUSY && a.saved.last_id==4);
    a.info_pending=0;m.fail_write=true;
    assert(ns_app_export_history(&a,102)==NS_IO && a.saved.last_id==5 && m.closes==2 && a.export_id==4);
    m.fail_write=false;m.fail_sync=true;
    assert(ns_app_export_history(&a,103)==NS_IO && m.closes==3);
    m.fail_sync=false;disk.fail_write=true;
    assert(ns_app_export_history(&a,104)==NS_IO && m.closes==3 && a.saved.last_id==6);
}
static void test_completion_navigation_and_recovery(void) {
    Mock m={0}; StoreMock disk={.fail_read=true}; NsPlatform radio=platform(&m); NsStore io=store(&disk);
    NsApplication a; NsFrame frame;
    assert(ns_app_init(&a,&radio,&io,(NsCapabilities){0})==NS_IO);
    /* Opening an unrelated menu must not falsely mark unreadable history saved. */
    a.menu=6; assert(ns_app_key(&a,NS_KEY_OK,0)==NS_OK);
    ns_app_frame(&a,&frame);
    assert(strstr(frame.rows[3],"unreadable") && strstr(frame.rows[5],"I/O"));
    assert(ns_app_key(&a,NS_KEY_BACK,0)==NS_OK);
    ns_app_frame(&a,&frame); assert(strstr(frame.rows[5],"SD:"));
    disk.fail_read=false; a.menu=6; assert(ns_app_key(&a,NS_KEY_OK,0)==NS_OK);
    assert(ns_app_key(&a,NS_KEY_OK,0)==NS_OK && a.persistence.loaded && a.storage_status==NS_OK);
    NsPage pages[]={NS_PAGE_RADIO,NS_PAGE_RADIO,NS_PAGE_RADIO,NS_PAGE_RADIO,
        NS_PAGE_SETTINGS,NS_PAGE_HISTORY,NS_PAGE_DIAGNOSTICS,NS_PAGE_FILTERS,NS_PAGE_EXPORTS,NS_PAGE_WIFI_OVERVIEW};
    for(unsigned i=0;i<NS_MENU_COUNT;i++) {
        a.page=NS_PAGE_HOME; a.menu=i;
        assert(ns_app_key(&a,NS_KEY_OK,10+i)==NS_OK && a.page==pages[i]);
        ns_app_frame(&a,&frame);
        if(i>=1 && i<=3) assert(strstr(frame.rows[5],"Unavailable"));
        if(i==9) {
            assert(strstr(frame.rows[2],"disabled"));
            unsigned acquired=m.acquired;
            assert(ns_app_key(&a,NS_KEY_OK,20)==NS_OK && a.page==NS_PAGE_RADIO && m.acquired==acquired);
        }
        assert(ns_app_key(&a,NS_KEY_BACK,30+i)==NS_OK && a.page==NS_PAGE_HOME && a.running);
    }
    assert(!m.owner && !m.uart_calls);
    a.menu=NS_MENU_COUNT-1;
    assert(ns_app_key(&a,NS_KEY_DOWN,40)==NS_OK && a.menu==NS_MENU_COUNT-1);
    assert(ns_app_save_settings(&a,&a.saved.settings)==NS_OK && !disk.writes);
    a.page=NS_PAGE_SETTINGS; a.setting=0;
    NsSettings settings=a.saved.settings; settings.timeout_ms=60000;
    assert(ns_app_save_settings(&a,&settings)==NS_OK);
    unsigned writes=disk.writes;
    for(unsigned i=0;i<50;i++) assert(ns_app_key(&a,NS_KEY_RIGHT,50+i)==NS_OK);
    assert(disk.writes==writes); /* no writes from repeat at a setting boundary */
    settings=a.saved.settings; settings.baud=9600;
    assert(ns_app_save_settings(&a,&settings)==NS_OK);
    a.setting=3; ns_app_frame(&a,&frame);
    assert(strstr(frame.rows[4],"9600") && strstr(frame.rows[4],"OK: 115200"));
    assert(ns_app_key(&a,NS_KEY_OK,100)==NS_OK && a.saved.settings.baud==115200 && !m.acquired);
    unsigned fixed_writes=disk.writes;
    assert(ns_app_key(&a,NS_KEY_OK,100)==NS_OK && disk.writes==fixed_writes);
    NsApplication reboot;
    assert(ns_app_init(&reboot,&radio,&io,(NsCapabilities){0})==NS_OK && reboot.saved.settings.baud==115200);
    a.page=NS_PAGE_HISTORY;
    assert(ns_app_key(&a,NS_KEY_RIGHT,100)==NS_OK && a.history_filter.radio==NS_RADIO_ESP);
    assert(ns_app_key(&a,NS_KEY_OK,100)==NS_OK && a.history_filter.failures_only);
    ns_app_frame(&a,&frame); assert(strstr(frame.rows[0],"ESP32 errors"));
    assert(ns_app_key(&a,NS_KEY_BACK,101)==NS_OK);
    assert(ns_app_key(&a,NS_KEY_BACK,102)==NS_OK && !a.running);
}
static void test_capture_deadlines(void) {
    for(unsigned kind=0;kind<2;kind++) for(unsigned failure=0;failure<2;failure++) {
        Mock m={0}; StoreMock disk={0}; NsPlatform radio=platform(&m); NsStore io=store(&disk); NsApplication a;
        assert(ns_app_init(&a,&radio,&io,(NsCapabilities){.nrf=true,.cc=true})==NS_ABSENT);
        NsRadio chosen=kind ? NS_RADIO_CC : NS_RADIO_NRF;
        uint32_t start=UINT32_MAX-100;
        assert(ns_app_start(&a,chosen,false,true,start)==NS_OK);
        assert(ns_app_step(&a,start+NS_CAPTURE_LIMIT_MS-1)==NS_OK && m.owner==chosen);
        unsigned observations=a.session.observations;
        m.fail_idle=kind && failure; m.fail_spi=!kind && failure;
        NsResult result=failure ? NS_IO : NS_OK;
        assert(ns_app_step(&a,start+NS_CAPTURE_LIMIT_MS)==result);
        assert(!m.owner && !m.ce && !a.session.logging && !a.history_dirty && a.session.capture_complete);
        assert(a.saved.history_count==1 && a.saved.history[0].result==result && a.session.observations==observations);
        assert(m.acquired==m.released && m.closes==1);
        NsFrame f;ns_app_frame(&a,&f);
        if(!failure) assert(strstr(f.rows[5],"Capture complete"));
        assert(ns_app_step(&a,start+NS_CAPTURE_LIMIT_MS+1)==NS_OK && m.released==1);
    }
}
static void test_information_log_completion_errors(void) {
    const char* reply="#info\nFirmware: Marauder\nVersion: v1.11.0\nHardware: Marauder v4\nESP-IDF: v5.5.1-710-g8410210c9a\nWSL Bypass: disabled\nSD Card: Not Connected\nSD Card Size: 0\nBattery Monitor: not supported\n> ";
    for(unsigned failure=0;failure<2;failure++) {
        Mock m={0}; StoreMock disk={0}; NsPlatform radio=platform(&m); NsStore io=store(&disk); NsApplication a;
        assert(ns_app_init(&a,&radio,&io,(NsCapabilities){.esp_uart=true,.esp_info_only=true})==NS_ABSENT);
        assert(ns_app_device_info(&a,0)==NS_OK);
        m.fail_sync=!failure; m.fail_close=failure;
        assert(ns_app_feed(&a,(const uint8_t*)reply,strlen(reply),1)==NS_IO);
        assert(!m.owner && m.closes==1 && !a.session.logging && !a.history_dirty && !a.info_validated);
        assert(a.saved.history_count==1 && a.saved.history[0].result==NS_IO);
        NsFrame f; ns_app_frame(&a,&f); assert(strstr(f.rows[5],"I/O") && !strstr(f.rows[5],"Connected"));
    }
}
int main(void) {
    assert(!strcmp(ns_result_name((NsResult)-1), "Unknown"));
    assert(!strcmp(ns_result_name((NsResult)(NS_FULL+1)), "Unknown"));
    assert(!strcmp(ns_result_name(NS_OK), "OK"));
    NsApplication invalid_key_app = {0}; invalid_key_app.running = true;
    assert(ns_app_key(&invalid_key_app, (NsKey)-1, 0) == NS_INVALID);
    assert(ns_app_key(&invalid_key_app, (NsKey)(NS_KEY_BACK+1), 0) == NS_INVALID);
    test_information_log_completion_errors(); test_completion_navigation_and_recovery(); test_capture_deadlines(); test_exports(); test_cc_explorer();
    test_persistence(); test_invalid_history_enums(); test_torn_writes(); test_app(); test_history_wrap_and_exhaustion(); test_recovery_and_cleanup(); test_readback_and_disabled();
    puts("PASS: application navigation, restart persistence, 420 torn writes, ID reservation and recovery");
}
