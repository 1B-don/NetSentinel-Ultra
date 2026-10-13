#include "../../include/netsentinel/cc_spi.h"
static NsResult status(const NsCcBus* bus, uint8_t address, uint8_t* value, uint32_t* count) {
    uint8_t tx[2] = {(uint8_t)(0xc0u | address), 0}, rx[2] = {0};
    NsResult r = bus->transfer(bus->ctx, tx, rx, sizeof(tx)); (*count)++;
    if(r != NS_OK) return r;
    if(rx[0] & 0x80) return NS_TIMEOUT; /* chip-ready must be bounded by adapter */
    *value = rx[1]; return NS_OK;
}
NsResult ns_cc_spi_diagnose(const NsCcBus* bus, NsCcDiagnostics* out) {
    if(!bus || !bus->transfer || !out) return NS_INVALID;
    NsCcDiagnostics d = {0}; uint8_t part2, version2, state2;
    NsResult r = status(bus, 0x30, &d.part, &d.transactions);
    if(r == NS_OK) r = status(bus, 0x31, &d.version, &d.transactions);
    if(r == NS_OK) r = status(bus, 0x30, &part2, &d.transactions);
    if(r == NS_OK) r = status(bus, 0x31, &version2, &d.transactions);
    if(r != NS_OK) return r;
    if(d.part != part2 || d.version != version2) return NS_IO;
    if(d.part != 0 || d.version == 0 || d.version == 0xff) return NS_ABSENT;
    /* Dynamic state can change mid-read. Bound retries and require two equal
     * values; do not treat a transitioning state as settled RX. */
    for(unsigned attempt = 0; attempt < 3; attempt++) {
        r = status(bus, 0x35, &d.state, &d.transactions);
        if(r == NS_OK) r = status(bus, 0x35, &state2, &d.transactions);
        if(r != NS_OK) return r;
        d.state &= 0x1f; state2 &= 0x1f;
        if(d.state == state2) {
            r = status(bus, 0x34, &d.rssi_raw, &d.transactions);
            if(r == NS_OK) *out = d;
            return r;
        }
    }
    return NS_TIMEOUT;
}
int16_t ns_cc_rssi_half_dbm(uint8_t raw, uint8_t offset) {
    int16_t signed_raw = raw < 128 ? raw : (int16_t)raw-256;
    return signed_raw - 2*(int16_t)offset;
}

NsResult ns_cc_spi_check_rx_policy(const NsCcBus* bus, uint8_t* out) {
    if(!bus || !bus->transfer || !out) return NS_INVALID;
    uint8_t values[2];
    for(unsigned i = 0; i < 2; i++) {
        const uint8_t tx[2] = {0x97, 0}; /* single-read MCSM1 configuration register */
        uint8_t rx[2] = {0};
        NsResult r = bus->transfer(bus->ctx, tx, rx, sizeof(tx));
        if(r != NS_OK) return r;
        if(rx[0] & 0x80) return NS_TIMEOUT;
        values[i] = rx[1];
    }
    if(values[0] != values[1]) return NS_IO;
    unsigned rxoff = (values[0] >> 2) & 3;
    if(rxoff != 0 && rxoff != 3) return NS_UNSUPPORTED;
    *out = values[0]; return NS_OK;
}
