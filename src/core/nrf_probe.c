#include "../../include/netsentinel/nrf_probe.h"
#include <string.h>

static bool expired(const NsNrfProbeIo* io, uint32_t start) {
    return (uint32_t)(io->now_ms(io->ctx) - start) >= NS_NRF_PROBE_BUDGET_MS;
}

static NsResult read_register(
    const NsNrfProbeIo* io,
    uint32_t start,
    uint8_t reg,
    uint8_t* status,
    uint8_t* value) {
    if(io->cancelled(io->ctx)) return NS_CANCELLED;
    uint32_t elapsed = (uint32_t)(io->now_ms(io->ctx) - start);
    if(elapsed >= NS_NRF_PROBE_BUDGET_MS) return NS_TIMEOUT;
    uint32_t remaining = NS_NRF_PROBE_BUDGET_MS - elapsed;
    uint32_t timeout = remaining < NS_NRF_PROBE_TRANSFER_MS ? remaining : NS_NRF_PROBE_TRANSFER_MS;
    const uint8_t tx[2] = {(uint8_t)(reg & 0x1f), 0xff}; /* R_REGISTER + dummy clocks */
    uint8_t rx[2] = {0};
    NsResult result = io->transfer(io->ctx, tx, rx, timeout);
    if(result != NS_OK) return result;
    if(expired(io, start)) return NS_TIMEOUT;
    *status = rx[0];
    *value = rx[1];
    return NS_OK;
}

static bool status_valid(uint8_t status) {
    return !(status & 0x80u) && ((status >> 1) & 0x07u) != 6u;
}

NsResult ns_nrf_probe_readonly(const NsNrfProbeIo* io, NsNrfProbeResult* output) {
    if(!output) return NS_INVALID;
    memset(output, 0, sizeof(*output));
    if(!io || !io->acquire || !io->release || !io->transfer || !io->now_ms || !io->cancelled)
        return NS_INVALID;

    const uint32_t start = io->now_ms(io->ctx);
    if(io->cancelled(io->ctx)) return NS_CANCELLED;
    NsResult result = io->acquire(io->ctx, NS_NRF_PROBE_BUDGET_MS);
    if(result != NS_OK) return result;

    NsNrfProbeResult candidate = {0};
    uint8_t transaction_status = 0;
    if(expired(io, start)) result = NS_TIMEOUT;
    else if(io->cancelled(io->ctx)) result = NS_CANCELLED;
    if(result == NS_OK)
        result = read_register(io, start, 0x07, &transaction_status, &candidate.status);
    if(result == NS_OK && (!status_valid(transaction_status) || !status_valid(candidate.status)))
        result = NS_ABSENT;
    if(result == NS_OK)
        result = read_register(io, start, 0x00, &transaction_status, &candidate.config);
    if(result == NS_OK && !status_valid(transaction_status)) result = NS_ABSENT;
    if(result == NS_OK && (candidate.config & 0x80u)) result = NS_ABSENT;
    if(result == NS_OK)
        result = read_register(io, start, 0x05, &transaction_status, &candidate.rf_channel);
    if(result == NS_OK && (!status_valid(transaction_status) || candidate.rf_channel > 125u))
        result = NS_ABSENT;
    if(result == NS_OK)
        result = read_register(io, start, 0x06, &transaction_status, &candidate.rf_setup);
    if(result == NS_OK && (!status_valid(transaction_status) || (candidate.rf_setup & 0xe0u)))
        result = NS_ABSENT;
    if(result == NS_OK && io->cancelled(io->ctx)) result = NS_CANCELLED;
    if(result == NS_OK && expired(io, start)) result = NS_TIMEOUT;

    io->release(io->ctx);
    if(result != NS_OK) return result;
    candidate.elapsed_ms = (uint32_t)(io->now_ms(io->ctx) - start);
    if(io->cancelled(io->ctx)) return NS_CANCELLED;
    if(candidate.elapsed_ms >= NS_NRF_PROBE_BUDGET_MS) return NS_TIMEOUT;
    *output = candidate;
    return NS_OK;
}
