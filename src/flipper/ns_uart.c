#include "ns_flipper.h"
#ifdef NS_GUI_STORAGE_ONLY
/* Build-only baseline: radio commands cannot acquire or touch UART hardware. */
NsResult ns_flipper_uart_acquire(void* ctx, NsRadio radio) {
    (void)ctx; (void)radio; return NS_UNSUPPORTED;
}
void ns_flipper_uart_release(void* ctx, NsRadio radio) { (void)ctx; (void)radio; }
NsResult ns_flipper_uart_write(void* ctx, const uint8_t* bytes, size_t count) {
    (void)ctx; (void)bytes; (void)count; return NS_UNSUPPORTED;
}
NsResult ns_flipper_uart_read(void* ctx, uint8_t* bytes, size_t cap, size_t* count) {
    (void)ctx; (void)bytes; (void)cap;
    if(count) *count = 0;
    return NS_UNSUPPORTED;
}
#else
#include <furi_hal_serial_control.h>
#include <string.h>
/* Single interrupt producer / single worker consumer. No allocation, parsing,
 * storage or GUI work in the ISR. A lost byte poisons the whole transaction. */
_Static_assert(ATOMIC_INT_LOCK_FREE == 2, "UART ISR requires lock-free unsigned atomics");
static void received(FuriHalSerialHandle* handle, FuriHalSerialRxEvent event, void* ctx) {
    NsFlipper* f = ctx;
    unsigned errors = FuriHalSerialRxEventFrameError | FuriHalSerialRxEventNoiseError |
        FuriHalSerialRxEventOverrunError | FuriHalSerialRxEventParityError;
    if((unsigned)event & errors) atomic_store(&f->rx_error, NS_IO);
    if(event & FuriHalSerialRxEventData) {
        while(furi_hal_serial_async_rx_available(handle)) {
            uint8_t byte = furi_hal_serial_async_rx(handle);
            unsigned head = atomic_load_explicit(&f->rx_head, memory_order_relaxed);
            unsigned tail = atomic_load_explicit(&f->rx_tail, memory_order_acquire);
            if(head-tail == NS_UART_CAPACITY) atomic_store(&f->rx_error, NS_FULL);
            else {
                f->rx[head % NS_UART_CAPACITY] = byte;
                atomic_store_explicit(&f->rx_head, head+1, memory_order_release);
            }
        }
    }
}
NsResult ns_flipper_uart_acquire(void* ctx, NsRadio radio) {
    NsFlipper* f = ctx;
    if(radio != NS_RADIO_ESP) return NS_UNSUPPORTED;
    if(f->serial) return NS_BUSY;
    f->serial = furi_hal_serial_control_acquire(FuriHalSerialIdUsart);
    if(!f->serial) return NS_BUSY; /* never force-disable another owner */
    atomic_store(&f->rx_head, 0); atomic_store(&f->rx_tail, 0); atomic_store(&f->rx_error, NS_OK);
    furi_hal_serial_init(f->serial, 115200);
    furi_hal_serial_async_rx_start(f->serial, received, f, true);
    return NS_OK;
}
void ns_flipper_uart_release(void* ctx, NsRadio radio) {
    NsFlipper* f = ctx;
    if(radio != NS_RADIO_ESP || !f->serial) return;
    furi_hal_serial_async_rx_stop(f->serial);
    furi_hal_serial_deinit(f->serial);
    furi_hal_serial_control_release(f->serial); f->serial = NULL;
}
NsResult ns_flipper_uart_write(void* ctx, const uint8_t* bytes, size_t count) {
    NsFlipper* f = ctx;
    if(!f->serial || !bytes || !count || count > 16) return NS_INVALID;
    bool permitted = (count == 5 && !memcmp(bytes, "info\n", 5)) ||
        (count == 5 && !memcmp(bytes, "help\n", 5)) ||
        (count == 9 && !memcmp(bytes, "stopscan\n", 9));
    if(f->capabilities.esp_wifi_verified && count == 12 && !memcmp(bytes, "sniffbeacon\n", 12)) permitted = true;
    if(!permitted) return NS_UNSUPPORTED;
    /* HAL has no deadline/error return. These short fixed commands avoid its
     * unbounded tx_wait_complete API; real SDK/hardware timing remains a gate. */
    furi_hal_serial_tx(f->serial, bytes, count); return NS_OK;
}
NsResult ns_flipper_uart_read(void* ctx, uint8_t* bytes, size_t cap, size_t* count) {
    NsFlipper* f = ctx;
    if(!count) return NS_INVALID;
    *count = 0;
    if(!f->serial || !bytes || !cap) return NS_INVALID;
    NsResult error = (NsResult)atomic_load(&f->rx_error); if(error != NS_OK) return error;
    unsigned tail = atomic_load_explicit(&f->rx_tail, memory_order_relaxed);
    unsigned head = atomic_load_explicit(&f->rx_head, memory_order_acquire);
    while(tail != head && *count < cap) bytes[(*count)++] = f->rx[(tail++) % NS_UART_CAPACITY];
    atomic_store_explicit(&f->rx_tail, tail, memory_order_release);
    error = (NsResult)atomic_load(&f->rx_error);
    if(error != NS_OK) *count = 0;
    return error;
}

#endif /* NS_GUI_STORAGE_ONLY */
