#ifndef NS_PERSISTENCE_H
#define NS_PERSISTENCE_H
#include "settings.h"
#include "session.h"
#define NS_STATE_CAPACITY 432
/* Slots are fixed application-owned files, separate from an open CSV handle.
 * read: NS_ABSENT only for a missing file; never for card/permission failures.
 * write: truncate ONLY the selected slot, full write, sync, close, then success.
 * Core reads back and verifies every successful write. No erase/rename required. */
typedef struct {
    void* ctx;
    NsResult (*read)(void*, unsigned slot, uint8_t*, size_t capacity, size_t* length);
    NsResult (*write)(void*, unsigned slot, const uint8_t*, size_t length);
} NsStore;
typedef struct {
    NsSettings settings;
    uint32_t last_id;
    NsHistory history[NS_HISTORY_LIMIT]; /* oldest first */
    size_t history_count;
} NsSavedState;
typedef struct { const NsStore* io; uint32_t generation; unsigned slot; bool loaded; } NsPersistence;
void ns_saved_defaults(NsSavedState*);
NsResult ns_persistence_load(NsPersistence*, const NsStore*, NsSavedState*);
NsResult ns_persistence_save(NsPersistence*, const NsSavedState*);
#endif
