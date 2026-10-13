#ifndef NS_PLATFORM_H
#define NS_PLATFORM_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef enum { NS_OK, NS_INVALID, NS_BUSY, NS_UNSUPPORTED, NS_IO, NS_ABSENT, NS_TIMEOUT, NS_FULL } NsResult;
typedef enum { NS_RADIO_NONE, NS_RADIO_ESP, NS_RADIO_NRF, NS_RADIO_CC } NsRadio;
typedef struct {
    uint8_t part, version, state, rssi_raw;
    uint32_t transactions;
} NsCcDiagnostics;
/* Adapter owns bus locking, finite transfer timeouts, CS and pin restoration.
 * release must lower CE, put the peripheral in a safe state and restore pins.
 * CC adapter must use external-only receive interfaces: never fall back internally. */
typedef struct {
    void* ctx;
    NsResult (*acquire)(void*, NsRadio);
    void (*release)(void*, NsRadio);
    NsResult (*spi)(void*, const uint8_t*, uint8_t*, size_t);
    void (*ce)(void*, bool);
    void (*delay_us)(void*, uint32_t);
    NsResult (*uart_write)(void*, const uint8_t*, size_t);
    NsResult (*cc_probe)(void*);
    NsResult (*cc_receive)(void*, uint32_t);
    NsResult (*cc_rssi)(void*, int16_t*); /* dBm, adapter validates RX state */
    NsResult (*cc_idle)(void*);
    NsResult (*file_create)(void*, const char*); /* exclusive create */
    NsResult (*file_write)(void*, const char*, size_t); /* full write or error */
    NsResult (*file_sync)(void*);
    NsResult (*file_close)(void*); /* close handle even on error; report status */
    /* Nonblocking, bounded receive. Set count<=capacity. Dropped RX bytes must
     * return NS_FULL, never silently splice fragments. Ownership is ESP only. */
    NsResult (*uart_read)(void*, uint8_t*, size_t capacity, size_t* count);
    /* Optional external-only raw status diagnostics. Complete snapshot or error.
     * No writes/strobes; settled receive must return MARCSTATE RX (0x0d). */
    NsResult (*cc_diagnose)(void*, NsCcDiagnostics*);
} NsPlatform;
#endif
