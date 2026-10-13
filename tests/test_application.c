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
    NsCapabilities caps = {true, true};
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
    assert(ns_app_init(&a, &radio, &io, (NsCapabilities){true, true}) == NS_ABSENT);
    for(unsigned i = 0; i < 24; i++) {
        assert(ns_app_start(&a, NS_RADIO_CC, false, true, i*100) == NS_OK);
        assert(ns_app_step(&a, i*100+1) == NS_OK);
        assert(ns_app_stop(&a, i*100+2) == NS_OK);
    }
    assert(ns_app_init(&a, &radio, &io, (NsCapabilities){true, true}) == NS_OK);
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
    assert(ns_app_save_settings(&a, &settings) == NS_FULL);
}
static void test_recovery_and_cleanup(void) {
    StoreMock disk = {.fail_read = true}; NsStore io = store(&disk);
    Mock m = {0}; NsPlatform radio = platform(&m); NsApplication a;
    assert(ns_app_init(&a, &radio, &io, (NsCapabilities){true, true}) == NS_IO);
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
    assert(ns_app_init(&a, &radio, &io, (NsCapabilities){true, true}) == NS_ABSENT);
    disk.corrupt_readback = true;
    assert(ns_app_start(&a, NS_RADIO_NRF, false, true, 0) == NS_IO && !m.acquired);
    disk.corrupt_readback = false;
    assert(ns_app_init(&a, &radio, &io, (NsCapabilities){true, true}) == NS_OK && a.saved.last_id == 1);
    assert(ns_app_start(&a, NS_RADIO_NRF, false, true, 0) == NS_OK && a.session.id == 2);
    assert(ns_app_stop(&a, 1) == NS_OK);
}
int main(void) {
    test_persistence(); test_torn_writes(); test_app(); test_history_wrap_and_exhaustion(); test_recovery_and_cleanup(); test_readback_and_disabled();
    puts("PASS: application navigation, restart persistence, 420 torn writes, ID reservation and recovery");
}
