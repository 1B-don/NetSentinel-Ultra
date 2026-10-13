#ifndef NS_MOCK_SERIAL_H
#define NS_MOCK_SERIAL_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
typedef struct FuriHalSerialHandle FuriHalSerialHandle;
typedef enum { FuriHalSerialRxEventData=1, FuriHalSerialRxEventIdle=2,
 FuriHalSerialRxEventFrameError=4, FuriHalSerialRxEventNoiseError=8,
 FuriHalSerialRxEventOverrunError=16, FuriHalSerialRxEventParityError=32 } FuriHalSerialRxEvent;
typedef void (*FuriHalSerialAsyncRxCallback)(FuriHalSerialHandle*, FuriHalSerialRxEvent, void*);
void furi_hal_serial_init(FuriHalSerialHandle*, uint32_t);
void furi_hal_serial_deinit(FuriHalSerialHandle*);
void furi_hal_serial_async_rx_start(FuriHalSerialHandle*, FuriHalSerialAsyncRxCallback, void*, bool);
void furi_hal_serial_async_rx_stop(FuriHalSerialHandle*);
bool furi_hal_serial_async_rx_available(FuriHalSerialHandle*);
uint8_t furi_hal_serial_async_rx(FuriHalSerialHandle*);
void furi_hal_serial_tx(FuriHalSerialHandle*, const uint8_t*, size_t);
void mock_serial_feed(const uint8_t*, size_t);
void mock_serial_error(FuriHalSerialRxEvent);
const char* mock_serial_commands(void);
#endif
