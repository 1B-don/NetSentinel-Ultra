/* Verify the actual GUI/storage-only source configuration; mock SDK, not ABI. */
#include "ns_flipper.h"
#include "mock_sdk.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int32_t netsentinel_app(void*);
int main(void) {
    mock_sdk_reset(); NsFlipper f;
    assert(ns_flipper_init(&f));
    assert(!f.capabilities.esp_uart && !f.capabilities.esp_wifi_verified);
    assert(!f.capabilities.nrf && !f.capabilities.cc && !f.serial);
    for(NsRadio r = NS_RADIO_ESP; r <= NS_RADIO_CC; r++)
        assert(f.platform.acquire(&f, r) == NS_UNSUPPORTED);
    uint8_t data[8]; size_t count = 99;
    assert(f.platform.uart_read(&f, data, sizeof(data), &count) == NS_UNSUPPORTED && count == 0);
    assert(f.platform.uart_write(&f, (const uint8_t*)"info\n", 5) == NS_UNSUPPORTED);
    NsSavedState saved, restored; ns_saved_defaults(&saved);
    NsPersistence persistence;
    assert(ns_persistence_load(&persistence, &f.store, &restored) == NS_ABSENT);
    saved.last_id = 7;
    assert(ns_persistence_save(&persistence, &saved) == NS_OK);
    assert(ns_persistence_load(&persistence, &f.store, &restored) == NS_OK);
    assert(restored.last_id == 7 && restored.settings.timeout_ms == saved.settings.timeout_ms);
    NsApplication app; assert(ns_app_init(&app, &f.platform, &f.store, f.capabilities) == NS_OK);
    app.selected = NS_RADIO_ESP; app.page = NS_PAGE_RADIO; NsFrame frame;
    ns_app_frame(&app, &frame);
    assert(!strcmp(frame.rows[4], "UART disabled in this build"));
    assert(!strcmp(frame.rows[5], "ESP32 adapter disabled"));
    app.status = NS_IO; ns_app_frame(&app, &frame);
    assert(!strcmp(frame.rows[5], ns_result_name(NS_IO))); /* preserve storage errors */
    ns_flipper_deinit(&f); assert(!mock_sdk_live() && !mock_sdk_records());
    mock_sdk_reset();
    InputEvent navigation[] = {
        {InputKeyDown, InputTypeShort}, {InputKeyDown, InputTypeShort},
        {InputKeyDown, InputTypeShort}, {InputKeyDown, InputTypeShort},
        {InputKeyOk, InputTypeShort}, {InputKeyRight, InputTypeShort},
        {InputKeyBack, InputTypeShort}, {InputKeyBack, InputTypeShort}};
    mock_sdk_events(navigation, sizeof(navigation)/sizeof(*navigation));
    assert(netsentinel_app(NULL) == 0);
    assert(!mock_sdk_live() && !mock_sdk_records());
    puts("PASS: actual GUI/storage-only build configuration, navigation, SD snapshot and disabled hardware (MOCK SDK)");
    return 0;
}
