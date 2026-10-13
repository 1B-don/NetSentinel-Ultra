#include "ns_flipper.h"
#include "mock_sdk.h"
#include "mock_platform.h"
#include <stdio.h>
int32_t netsentinel_app(void*);
static Mock board;
static unsigned binds, unbinds;
static bool incomplete, fail_bind;
static NsResult acquire_failure;
static NsResult checked_acquire(void* ctx, NsRadio radio) {
    if(acquire_failure != NS_OK) return acquire_failure;
    return acquire(ctx, radio);
}
static void safe_release(void* ctx, NsRadio radio) {
    Mock* m = ctx; m->frequency = 0; m->ce = false; release(ctx, radio);
}
bool ns_flipper_board_bind(NsBoardPort* port) {
    binds++; port->io = platform(&board); port->io.release = safe_release;
    port->io.acquire = checked_acquire;
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
    acquire_failure = NS_OK;
}
static void ownership_recovery_and_repeated_cancel(void) {
    reset();
    NsFlipper f; assert(ns_flipper_init(&f));
    const NsResult failures[] = {NS_BUSY, NS_IO, NS_TIMEOUT, NS_CANCELLED};
    const NsRadio radios[] = {NS_RADIO_NRF, NS_RADIO_CC};
    for(unsigned cycle = 0; cycle < 24; cycle++) {
        NsRadio radio = radios[cycle % 2];
        NsRadio other = radios[(cycle + 1) % 2];
        acquire_failure = failures[cycle % 4];
        unsigned acquired_before = board.acquired, released_before = board.released;
        assert(f.platform.acquire(&f, radio) == acquire_failure);
        assert(f.owner == NS_RADIO_NONE && board.owner == NS_RADIO_NONE);
        f.platform.release(&f, radio);
        assert(board.acquired == acquired_before && board.released == released_before);
        acquire_failure = NS_OK;

        /* A session stop is cancellation between finite provider calls. */
        NsSession session; ns_session_init(&session, &f.platform, NULL);
        assert(ns_session_start(&session, radio, false, true, cycle * 10) == NS_OK);
        assert(f.platform.acquire(&f, other) == NS_BUSY);
        f.platform.release(&f, other);
        assert(f.owner == radio && board.owner == radio);
        ns_session_stop(&session, cycle * 10 + 1);
        assert(session.active == NS_RADIO_NONE && f.owner == NS_RADIO_NONE);
        assert(!board.owner && !board.ce && !board.frequency);
        assert(board.acquired == board.released);
        ns_session_stop(&session, cycle * 10 + 2);
        assert(board.acquired == board.released && board.released == released_before + 1);
    }
    ns_flipper_deinit(&f);
    assert(binds == unbinds && !mock_sdk_live() && !mock_sdk_records());
}
static void port_lifecycle(void) {
    reset(); NsFlipper f; assert(ns_flipper_init(&f));
    assert(f.capabilities.nrf && f.capabilities.cc && f.capabilities.esp_uart && !f.capabilities.esp_wifi_verified);
    assert(f.platform.ctx == &f && f.store.ctx == &f);
    assert(f.platform.acquire(&f, NS_RADIO_CC) == NS_OK);
    assert(f.platform.acquire(&f, NS_RADIO_ESP) == NS_BUSY);
    assert(f.platform.acquire(&f, NS_RADIO_NRF) == NS_BUSY);
    assert(f.platform.cc_receive(&f, 433920000, NS_CC_TUNE_TIMEOUT_MS) == NS_OK);
    uint8_t tx[] = {0, 0}, rx[2]; assert(f.platform.spi(&f, tx, rx, 2, NS_NRF_SPI_TIMEOUT_MS) == NS_INVALID);
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
static void receive_only_boundary(void) {
    reset(); NsFlipper f; assert(ns_flipper_init(&f));
    assert(f.platform.acquire(&f, NS_RADIO_NRF) == NS_OK);
    uint8_t tx[2], rx[2];
    /* Exhaustively reject every command/value outside the scanner envelope. */
    for(unsigned command = 0; command < 256; command++) {
        for(unsigned value = 0; value < 256; value++) {
            tx[0] = (uint8_t)command; tx[1] = (uint8_t)value;
            bool allowed = command == 0 || command == 1 || command == 2 ||
                command == 4 || command == 5 || command == 6 || command == 9 ||
                (command == 0x20 && (value == 1 || value == 3)) ||
                ((command == 0x21 || command == 0x22 || command == 0x24 || command == 0x26) && value == 0) ||
                (command == 0x25 && value < NS_NRF_CHANNELS);
            if(!allowed) assert(f.platform.spi(&f, tx, rx, 2, NS_NRF_SPI_TIMEOUT_MS) == NS_UNSUPPORTED);
        }
    }
    assert(f.platform.spi(&f, NULL, rx, 2, NS_NRF_SPI_TIMEOUT_MS) == NS_INVALID);
    assert(f.platform.spi(&f, tx, NULL, 2, NS_NRF_SPI_TIMEOUT_MS) == NS_INVALID);
    assert(f.platform.spi(&f, tx, rx, 1, NS_NRF_SPI_TIMEOUT_MS) == NS_INVALID);
    assert(f.platform.spi(&f, tx, rx, 3, NS_NRF_SPI_TIMEOUT_MS) == NS_INVALID);
    assert(board.transfers == 0); /* rejected transfers never reached SPI */
    /* The real portable RX driver must still run through the guarded port. */
    f.platform.release(&f, NS_RADIO_NRF);
    NsNrf scanner = {0};
    assert(ns_nrf_open(&scanner, &f.platform, true) == NS_OK);
    for(unsigned ch = 0; ch < NS_NRF_CHANNELS; ch++)
        assert(ns_nrf_sample(&scanner, (uint8_t)ch, 200) == NS_OK);
    assert(ns_nrf_close(&scanner) == NS_OK);
    ns_flipper_deinit(&f);
    assert(!board.owner && !board.ce && !mock_sdk_live() && !mock_sdk_records());
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
int main(void) { port_lifecycle(); ownership_recovery_and_repeated_cancel(); receive_only_boundary(); shell_radio_lifecycle(); puts("PASS: typed Flipper board bind, shared ownership, partial init, real GUI callback cancellation and forced cleanup (MOCK API)"); }
