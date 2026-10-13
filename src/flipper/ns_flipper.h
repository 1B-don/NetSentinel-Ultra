#ifndef NS_FLIPPER_H
#define NS_FLIPPER_H
#include "../../include/netsentinel/application.h"
#include <storage/storage.h>
#include <furi_hal_serial.h>
#include <stdatomic.h>
#define NS_UART_CAPACITY 4096
typedef struct {
    NsPlatform io; /* only radio callbacks used; context is private to board */
    bool nrf, cc;
} NsBoardPort;
typedef struct {
    FuriHalSerialHandle* serial;
    uint8_t rx[NS_UART_CAPACITY];
    atomic_uint rx_head, rx_tail, rx_error;
    Storage* storage;
    File* log;
    bool log_open;
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
