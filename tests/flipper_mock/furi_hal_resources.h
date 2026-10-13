#ifndef NS_MOCK_RESOURCES_H
#define NS_MOCK_RESOURCES_H
#include <stdint.h>
#include <stdbool.h>
typedef struct { int32_t connector; } GpioPin;
int32_t furi_hal_resources_get_ext_pin_number(const GpioPin*);
void mock_serial_pin_mismatch(bool);
#endif
