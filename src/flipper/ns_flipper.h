#ifndef NS_FLIPPER_H
#define NS_FLIPPER_H
#include "../../include/netsentinel/application.h"
#include <storage/storage.h>
#ifndef NS_GUI_STORAGE_ONLY
#include <furi_hal_serial.h>
#endif
#if (defined(NS_GUI_STORAGE_ONLY) && defined(NS_ENABLE_BOARD_PORT)) || \
    (defined(NS_ESP_INFO_ONLY) && (defined(NS_GUI_STORAGE_ONLY) || defined(NS_ENABLE_BOARD_PORT)))
#error "GUI/storage build cannot enable hardware board ports"
#endif
#if defined(NS_ENABLE_BOARD_PORT) && !defined(NS_TEST_BOARD_PORT) && \
    !defined(NS_VERIFIED_ULTRA_V3_NRF24_ROUTE) && \
    !(defined(NS_VERIFIED_ULTRA_V3_CC1101_ROUTE) && defined(NS_VERIFIED_MOMENTUM_CC1101_PROVIDER))
#error "physical radio board port requires a verified Ultra V3 route; host mock tests must define NS_TEST_BOARD_PORT"
#endif
#include <stdatomic.h>
#define NS_UART_CAPACITY 4096
typedef struct {
    NsPlatform io; /* only radio callbacks used; context is private to board */
    bool nrf, cc;
} NsBoardPort;
typedef struct {
#ifdef NS_GUI_STORAGE_ONLY
    void* serial; /* always NULL; no serial HAL declarations/imports */
#else
    FuriHalSerialHandle* serial;
#endif
    uint8_t rx[NS_UART_CAPACITY];
    atomic_uint rx_head, rx_tail, rx_error;
    Storage* storage;
    File* log;
    bool log_open, info_sent;
    unsigned rx_total; /* ISR-owned during each acquired transaction */
    NsBoardPort board;
    NsRadio owner;
    NsPlatform platform;
    NsStore store;
    NsCapabilities capabilities;
} NsFlipper;
NsResult ns_flipper_uart_acquire(void*, NsRadio);
void ns_flipper_uart_release(void*, NsRadio);
NsResult ns_flipper_uart_write(void*, const uint8_t*, size_t);
NsResult ns_flipper_uart_read(void*, uint8_t*, size_t, size_t*);
bool ns_flipper_board_valid(const NsBoardPort*);
void ns_flipper_radio_release(void*, NsRadio);
bool ns_flipper_init(NsFlipper*);
void ns_flipper_deinit(NsFlipper*);
/* Optional authorized port: bind populates this isolated board object only.
 * No power/pin acquisition in bind. release guarantees peripheral safety.
 * Partial bind must be safe to unbind. No internal-radio fallback. */
#ifdef NS_ENABLE_BOARD_PORT
bool ns_flipper_board_bind(NsBoardPort*);
void ns_flipper_board_unbind(NsBoardPort*);
#endif
#endif
