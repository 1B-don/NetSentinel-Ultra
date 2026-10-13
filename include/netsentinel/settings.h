#ifndef NS_SETTINGS_H
#define NS_SETTINGS_H
#include "platform.h"
typedef struct { uint32_t baud, timeout_ms, dwell_us, cc_frequency_hz; } NsSettings;
NsSettings ns_settings_default(void);
NsResult ns_settings_parse(const char*, NsSettings*);
NsResult ns_settings_format(const NsSettings*, char*, size_t);
/* Normalizes activity into screen-height bars without inventing RSSI. */
uint8_t ns_graph_height(uint32_t hits, uint32_t samples, uint8_t height);
#endif
