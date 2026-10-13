#ifndef NS_NRF_PROBE_H
#define NS_NRF_PROBE_H

#include "platform.h"

/* A status-only SPI sanity check. This does not identify or authenticate a
 * chip, configure registers, raise CE, enter RX, or read payloads. */
#define NS_NRF_PROBE_BUDGET_MS 100u
#define NS_NRF_PROBE_TRANSFER_MS 20u

typedef struct {
    void* ctx;
    /* Must honor the supplied bound. SDK furi_hal_spi_acquire() cannot. */
    NsResult (*acquire)(void*, uint32_t timeout_ms);
    /* Must restore ownership promptly; result deadline is checked afterward. */
    void (*release)(void*);
    /* One two-byte transaction; must honor timeout_ms. */
    NsResult (*transfer)(void*, const uint8_t tx[2], uint8_t rx[2], uint32_t timeout_ms);
    uint32_t (*now_ms)(void*);
    bool (*cancelled)(void*);
} NsNrfProbeIo;

typedef struct {
    uint8_t status, config, rf_channel, rf_setup;
    uint32_t elapsed_ms;
} NsNrfProbeResult;

/* Reads only STATUS, CONFIG, RF_CH and RF_SETUP. NS_OK means values are
 * register-plausible, not that an authentic nRF24 was identified. */
NsResult ns_nrf_probe_readonly(const NsNrfProbeIo*, NsNrfProbeResult*);

#endif
