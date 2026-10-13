#ifndef NS_APPLICATION_H
#define NS_APPLICATION_H
#include "persistence.h"
typedef enum { NS_KEY_UP, NS_KEY_DOWN, NS_KEY_LEFT, NS_KEY_RIGHT, NS_KEY_OK, NS_KEY_BACK } NsKey;
typedef enum { NS_PAGE_HOME, NS_PAGE_RADIO, NS_PAGE_SELECTOR, NS_PAGE_SETTINGS, NS_PAGE_HISTORY, NS_PAGE_DIAGNOSTICS } NsPage;
typedef struct { bool nrf, cc; } NsCapabilities;
/* Backend is supplied by an authorized SDK port. No ESP profile is present in
 * the Flipper application; firmware-specific passive commands remain disabled. */
typedef struct {
    NsSession session;
    NsSavedState saved;
    NsPersistence persistence;
    NsCapabilities capabilities;
    NsPage page;
    unsigned menu, setting, history_index;
    NsRadio selected;
    bool ble, running, history_dirty;
    NsResult status;
} NsApplication;
typedef struct {
    char rows[6][40];
    uint8_t bars[NS_NRF_CHANNELS];
    size_t bar_count;
} NsFrame;
NsResult ns_app_init(NsApplication*, const NsPlatform*, const NsStore*, NsCapabilities);
NsResult ns_app_reload(NsApplication*);
NsResult ns_app_start(NsApplication*, NsRadio, bool ble, bool selector_confirmed, uint32_t now_ms);
NsResult ns_app_stop(NsApplication*, uint32_t now_ms);
NsResult ns_app_save_settings(NsApplication*, const NsSettings*);
NsResult ns_app_step(NsApplication*, uint32_t now_ms);
NsResult ns_app_feed(NsApplication*, const uint8_t*, size_t, uint32_t now_ms);
NsResult ns_app_key(NsApplication*, NsKey, uint32_t now_ms);
void ns_app_frame(const NsApplication*, NsFrame*);
const char* ns_result_name(NsResult);
#endif
