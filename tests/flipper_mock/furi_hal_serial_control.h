#include "furi_hal_serial.h"
typedef enum { FuriHalSerialIdUsart } FuriHalSerialId;
FuriHalSerialHandle* furi_hal_serial_control_acquire(FuriHalSerialId);
void furi_hal_serial_control_release(FuriHalSerialHandle*);
