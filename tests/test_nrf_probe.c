#include "netsentinel/nrf_probe.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint32_t now;
    uint8_t regs[8];
    NsResult acquire_result, transfer_result;
    bool owned, cancelled;
    unsigned acquired, released, transfers, cancel_after;
    uint32_t last_acquire_timeout, last_transfer_timeout, step_ms, acquire_advance_ms, release_advance_ms;
    bool ce_changed, register_written;
} ProbeMock;

static NsResult probe_acquire(void* ctx, uint32_t timeout_ms) {
    ProbeMock* m = ctx;
    assert(timeout_ms && timeout_ms <= NS_NRF_PROBE_BUDGET_MS);
    m->last_acquire_timeout = timeout_ms;
    if(m->owned) return NS_BUSY;
    if(m->acquire_result != NS_OK) return m->acquire_result;
    if(m->acquire_advance_ms > timeout_ms) { m->now += timeout_ms; return NS_TIMEOUT; }
    m->now += m->acquire_advance_ms;
    m->owned = true;
    m->acquired++;
    return NS_OK;
}
static void probe_release(void* ctx) {
    ProbeMock* m = ctx;
    assert(m->owned);
    m->now += m->release_advance_ms;
    m->owned = false;
    m->released++;
}
static NsResult probe_transfer(void* ctx, const uint8_t tx[2], uint8_t rx[2], uint32_t timeout_ms) {
    ProbeMock* m = ctx;
    assert(m->owned && tx && rx && timeout_ms && timeout_ms <= NS_NRF_PROBE_TRANSFER_MS);
    assert(tx[0] == 0x07 || tx[0] == 0x00 || tx[0] == 0x05 || tx[0] == 0x06);
    assert(tx[1] == 0xff); /* dummy clocks only; no register writes */
    m->last_transfer_timeout = timeout_ms;
    m->transfers++;
    if(m->step_ms > timeout_ms) { m->now += timeout_ms; return NS_TIMEOUT; }
    m->now += m->step_ms;
    if(m->cancel_after && m->transfers >= m->cancel_after) m->cancelled = true;
    if(m->transfer_result != NS_OK) return m->transfer_result;
    rx[0] = 0x0e;
    rx[1] = m->regs[tx[0]];
    return NS_OK;
}
static uint32_t probe_now(void* ctx) { return ((ProbeMock*)ctx)->now; }
static bool probe_cancelled(void* ctx) { return ((ProbeMock*)ctx)->cancelled; }
static NsNrfProbeIo probe_io(ProbeMock* m) {
    return (NsNrfProbeIo){m, probe_acquire, probe_release, probe_transfer, probe_now, probe_cancelled};
}
static void reset(ProbeMock* m) {
    memset(m, 0, sizeof(*m));
    m->regs[7] = 0x0e;
    m->regs[0] = 0x08;
    m->regs[5] = 42;
    m->regs[6] = 0x07;
}

static void test_read_only_success_and_repeat(void) {
    ProbeMock m; reset(&m);
    NsNrfProbeIo io = probe_io(&m);
    for(unsigned i = 0; i < 12; i++) {
        NsNrfProbeResult out = {0};
        assert(ns_nrf_probe_readonly(&io, &out) == NS_OK);
        assert(out.status == 0x0e && out.config == 0x08 && out.rf_channel == 42 && out.rf_setup == 0x07);
        assert(out.elapsed_ms <= NS_NRF_PROBE_BUDGET_MS);
        assert(!m.owned && !m.register_written && !m.ce_changed);
    }
    assert(m.acquired == 12 && m.released == 12 && m.transfers == 48);
    assert(m.last_acquire_timeout == NS_NRF_PROBE_BUDGET_MS);
    assert(m.last_transfer_timeout <= NS_NRF_PROBE_TRANSFER_MS);
}

static void test_missing_invalid_values_and_cleanup(void) {
    ProbeMock m; reset(&m);
    NsNrfProbeIo io = probe_io(&m);
    NsNrfProbeResult out;

    m.regs[7] = 0xff;
    assert(ns_nrf_probe_readonly(&io, &out) == NS_ABSENT);
    assert(!m.owned && m.released == 1 && !out.status && !out.rf_channel);

    const struct { uint8_t reg, value; } bad[] = {
        {0, 0x80},       /* CONFIG reserved bit */
        {5, 126},        /* RF_CH range */
        {6, 0x20},       /* RF_SETUP reserved bits */
    };
    for(size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        reset(&m); m.regs[bad[i].reg] = bad[i].value;
        out = (NsNrfProbeResult){0xa5, 0xa5, 0xa5, 0xa5, 0xa5a5a5a5};
        assert(ns_nrf_probe_readonly(&io, &out) == NS_ABSENT);
        assert(!m.owned && m.acquired == m.released);
        assert(!out.status && !out.config && !out.rf_channel && !out.rf_setup && !out.elapsed_ms);
    }
    reset(&m); m.regs[7] = 0x80;
    assert(ns_nrf_probe_readonly(&io, &out) == NS_ABSENT && !m.owned);
    reset(&m); m.regs[7] = 0x0c; /* RX_P_NO == reserved pipe 6 */
    assert(ns_nrf_probe_readonly(&io, &out) == NS_ABSENT && !m.owned);
}

static void test_timeout_contention_cancel_and_wrap(void) {
    ProbeMock m; reset(&m);
    NsNrfProbeIo io = probe_io(&m);
    NsNrfProbeResult out = {0};
    m.acquire_result = NS_BUSY;
    assert(ns_nrf_probe_readonly(&io, &out) == NS_BUSY && !m.released && !m.transfers);
    m.acquire_result = NS_TIMEOUT;
    assert(ns_nrf_probe_readonly(&io, &out) == NS_TIMEOUT && !m.released);

    m.acquire_result = NS_OK; m.transfer_result = NS_TIMEOUT;
    assert(ns_nrf_probe_readonly(&io, &out) == NS_TIMEOUT && m.acquired == m.released && !m.owned);

    reset(&m); io = probe_io(&m); m.cancelled = true;
    assert(ns_nrf_probe_readonly(&io, &out) == NS_CANCELLED && !m.acquired);
    reset(&m); io = probe_io(&m); m.cancel_after = 1;
    assert(ns_nrf_probe_readonly(&io, &out) == NS_CANCELLED);
    assert(m.transfers == 1 && m.acquired == m.released && !m.owned);

    reset(&m); io = probe_io(&m); m.now = UINT32_MAX - 3u; m.step_ms = 1;
    assert(ns_nrf_probe_readonly(&io, &out) == NS_OK);
    assert(out.elapsed_ms == 4 && m.acquired == m.released);

    reset(&m); io = probe_io(&m); m.acquire_advance_ms = 95; m.step_ms = 10;
    assert(ns_nrf_probe_readonly(&io, &out) == NS_TIMEOUT);
    assert(m.transfers == 1 && m.acquired == m.released && !m.owned);

    reset(&m); io = probe_io(&m); m.release_advance_ms = NS_NRF_PROBE_BUDGET_MS;
    assert(ns_nrf_probe_readonly(&io, &out) == NS_TIMEOUT);
    assert(m.transfers == 4 && m.acquired == m.released && !m.owned && !out.elapsed_ms);
}

static void test_invalid_contracts(void) {
    ProbeMock m; reset(&m);
    NsNrfProbeIo io = probe_io(&m);
    NsNrfProbeResult out;
    assert(ns_nrf_probe_readonly(NULL, &out) == NS_INVALID);
    assert(ns_nrf_probe_readonly(&io, NULL) == NS_INVALID);
    io.transfer = NULL;
    assert(ns_nrf_probe_readonly(&io, &out) == NS_INVALID && !m.acquired);
}

int main(void) {
    test_read_only_success_and_repeat();
    test_missing_invalid_values_and_cleanup();
    test_timeout_contention_cancel_and_wrap();
    test_invalid_contracts();
    puts("PASS: NRF24 read-only probe budget, validation, cancellation and ownership");
}
