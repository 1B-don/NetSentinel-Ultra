#ifndef NS_OBSERVATION_H
#define NS_OBSERVATION_H
#include "persistence.h"
typedef struct { uint8_t channel; int16_t minimum_rssi; uint32_t maximum_age_ms; } NsObservationFilter;
typedef struct { NsRadio radio; bool failures_only; } NsHistoryFilter;
typedef struct {
    uint8_t peak_channel, quiet_channel;
    unsigned measured_channels, peak_percent, quiet_percent;
    uint64_t hits, samples;
} NsActivitySummary;
NsObservationFilter ns_observation_filter_default(void);
/* Stable indices into RAM observations. Never returns invalid channel/RSSI.
 * Outputs unchanged on invalid input; NS_FULL explicitly rejects small buffers. */
NsResult ns_observation_select(const NsAccessPoint*, const uint32_t* seen, size_t count,
    const NsObservationFilter*, uint32_t now, uint8_t* indices, size_t capacity, size_t* selected);
const NsHistory* ns_history_select(const NsSavedState* state, const NsHistoryFilter*, size_t ordinal);
/* Only channels with measurements participate; RPD is activity, not RSSI. */
NsResult ns_activity_summary(const NsNrf*, NsActivitySummary*);
#endif
