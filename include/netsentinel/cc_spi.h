#ifndef NS_CC_SPI_H
#define NS_CC_SPI_H
#include "platform.h"
/* Transport must own the EXTERNAL CC bus, bound chip-ready/transfer waits,
 * deassert CS even on failure, and restore bus state. No GPIO assumptions here. */
typedef struct {
    void* ctx;
    NsResult (*transfer)(void*, const uint8_t*, uint8_t*, size_t);
} NsCcBus;
/* Read-only status headers only: no strobes, register writes or TX FIFO. */
NsResult ns_cc_spi_diagnose(const NsCcBus*, NsCcDiagnostics*);
/* Read MCSM1 twice. Reject RXOFF transitions toward TX/FSTXON; accept only
 * IDLE/stay-RX. This checks one configuration condition, not full passivity.
 * Output is unchanged on error. No board pins, initialization or writes. */
NsResult ns_cc_spi_check_rx_policy(const NsCcBus*, uint8_t* mcsm1);
/* Explicit modem/board RSSI offset in dB; result retains half-dB resolution. */
int16_t ns_cc_rssi_half_dbm(uint8_t raw, uint8_t offset_db);
#endif
