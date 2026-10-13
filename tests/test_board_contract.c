#include "ns_flipper.h"
#include "mock_sdk.h"
#include "mock_platform.h"
#include <stdio.h>
int32_t netsentinel_app(void*);
static Mock board;
static unsigned binds, unbinds;
static bool incomplete, fail_bind;
static void safe_release(void* ctx, NsRadio radio) {
    Mock* m = ctx; m->frequency = 0; m->ce = false; release(ctx, radio);
}
bool ns_flipper_board_bind(NsBoardPort* port) {
    binds++; port->io = platform(&board); port->io.release = safe_release;
    port->nrf = port->cc = true;
    if(incomplete) port->io.cc_rssi = NULL;
    return !fail_bind;
}
void ns_flipper_board_unbind(NsBoardPort* port) {
    if(port->io.ctx) unbinds++;
    assert(!board.owner && !board.ce); memset(port, 0, sizeof(*port));
}
static void reset(void) {
    mock_sdk_reset(); memset(&board, 0, sizeof(board)); board.rssi = -70;
    incomplete = fail_bind = false; binds = unbinds = 0;
}
static void port_lifecycle(void) {
    reset(); NsFlipper f; assert(ns_flipper_init(&f));
    assert(f.capabilities.nrf && f.capabilities.cc && f.capabilities.esp_uart && !f.capabilities.esp_wifi_verified);
    assert(f.platform.ctx == &f && f.store.ctx == &f);
    assert(f.platform.acquire(&f, NS_RADIO_CC) == NS_OK);
    assert(f.platform.acquire(&f, NS_RADIO_ESP) == NS_BUSY);
    assert(f.platform.acquire(&f, NS_RADIO_NRF) == NS_BUSY);
    assert(f.platform.cc_receive(&f, 433920000) == NS_OK);
    uint8_t tx[] = {0, 0}, rx[2]; assert(f.platform.spi(&f, tx, rx, 2) == NS_INVALID);
    f.platform.release(&f, NS_RADIO_NRF); assert(board.owner == NS_RADIO_CC);
    ns_flipper_deinit(&f); assert(!board.owner && !board.frequency && binds == unbinds);
    assert(!mock_sdk_live() && !mock_sdk_records());
    reset(); assert(ns_flipper_init(&f));
    assert(f.platform.acquire(&f, NS_RADIO_ESP) == NS_OK);
    assert(f.platform.acquire(&f, NS_RADIO_CC) == NS_BUSY);
    ns_flipper_deinit(&f); assert(!f.serial && !f.owner && binds == unbinds);
    assert(!mock_sdk_live() && !mock_sdk_records());
    reset(); incomplete = true; assert(!ns_flipper_init(&f));
    assert(binds == 1 && unbinds == 1 && !mock_sdk_live() && !mock_sdk_records());
    reset(); fail_bind = true; assert(!ns_flipper_init(&f));
    assert(binds == 1 && unbinds == 1 && !mock_sdk_live() && !mock_sdk_records());
}
static void shell_radio_lifecycle(void) {
    reset(); InputEvent cc[] = {
        {InputKeyDown, InputTypeShort}, {InputKeyDown, InputTypeShort}, {InputKeyDown, InputTypeShort},
        {InputKeyOk, InputTypeShort}, {InputKeyOk, InputTypeShort}, {InputKeyOk, InputTypeShort},
        {InputKeyBack, InputTypeShort}, {InputKeyBack, InputTypeShort}};
    mock_sdk_events(cc, sizeof(cc)/sizeof(*cc));
    assert(netsentinel_app(NULL) == 0 && board.acquired == 1 && board.released == 1);
    assert(!board.frequency && !board.writes && binds == unbinds && !mock_sdk_live() && !mock_sdk_records());
    reset(); InputEvent nrf[] = {
        {InputKeyDown, InputTypeShort}, {InputKeyDown, InputTypeShort},
        {InputKeyOk, InputTypeShort}, {InputKeyOk, InputTypeShort}, {InputKeyOk, InputTypeShort},
        {InputKeyBack, InputTypeShort}, {InputKeyBack, InputTypeShort}};
    board.fail_spi = true; mock_sdk_events(nrf, sizeof(nrf)/sizeof(*nrf));
    assert(netsentinel_app(NULL) == 1 && board.acquired == board.released && !board.ce);
    assert(binds == unbinds && !mock_sdk_live() && !mock_sdk_records());
    reset(); mock_sdk_events(cc, sizeof(cc)/sizeof(*cc)); mock_sdk_fail_mutex(11);
    /* Publication/draw mutex failure while CC RX is active unwinds forcibly. */
    assert(netsentinel_app(NULL) == 1 && board.acquired == board.released && !board.frequency);
    assert(binds == unbinds && !mock_sdk_live() && !mock_sdk_records());
}
int main(void) { port_lifecycle(); shell_radio_lifecycle(); puts("PASS: typed Flipper board bind, shared ownership, partial init, real GUI callback cancellation and forced cleanup (MOCK API)"); }
