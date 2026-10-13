#include "ns_flipper.h"
#include "mock_sdk.h"
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
int main(void) { test_adapter(); test_shell(); puts("PASS: Flipper shell/storage contract shims, exclusive CSV, GUI lifecycle and allocation unwind (not SDK ABI)"); }
