#pragma once
#include "ap_parser.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t new_aps;
    uint8_t missing_aps;
    uint8_t channel_changes;
} NsDiff;

void ns_channel_counts(const NsAccessPoint* aps, size_t count, uint8_t channels[14]);
NsDiff ns_diff(const NsAccessPoint* before, size_t before_count,
               const NsAccessPoint* after, size_t after_count);
/* Writes a CSV record into buffer with RFC4180 string quoting. false on overflow. */
bool ns_csv_ap(const NsAccessPoint* ap, char* out, size_t cap);
