#ifndef NS_APPLICATION_H
#define NS_APPLICATION_H
#include "observation.h"
typedef enum { NS_KEY_UP, NS_KEY_DOWN, NS_KEY_LEFT, NS_KEY_RIGHT, NS_KEY_OK, NS_KEY_BACK } NsKey;
typedef enum { NS_PAGE_HOME, NS_PAGE_RADIO, NS_PAGE_SELECTOR, NS_PAGE_SETTINGS, NS_PAGE_HISTORY, NS_PAGE_DIAGNOSTICS, NS_PAGE_FILTERS, NS_PAGE_EXPORTS, NS_PAGE_WIFI_OVERVIEW } NsPage;
#define NS_MENU_COUNT 10u
typedef struct { bool nrf, cc, esp_uart, esp_wifi_verified, esp_info_only; } NsCapabilities;
/* Wi-Fi requires installed-build command/output verification in addition to
 * a UART port. BLE is unsupported. Information queries do not scan. */
typedef struct {
    NsSession session;
    NsSavedState saved;
    NsPersistence persistence;
    NsCapabilities capabilities;
    NsPage page;
    unsigned menu, setting, history_index, filter_setting, diagnostic_view;
    NsObservationFilter observation_filter;
    NsHistoryFilter history_filter;
    uint32_t now_ms, export_id;
    NsRadio selected;
    bool ble, running, history_dirty, exit_requested, wifi_details, cc_details, nrf_details;
    unsigned ap_index, info_pending, info_view;
    /* Volatile bounded diagnostics; never serialized or copied from raw UART. */
    bool info_timing, info_validated, info_cancelled, info_sd_connected;
    uint32_t info_elapsed_ms, info_success_id;
    NsAccessPoint baseline[NS_AP_LIMIT];
    size_t baseline_count;
    NsResult status, storage_status;
} NsApplication;
typedef struct {
    char rows[6][40];
    uint8_t bars[NS_NRF_CHANNELS];
    size_t bar_count;
    bool cc_plot;
} NsFrame;
NsResult ns_app_init(NsApplication*, const NsPlatform*, const NsStore*, NsCapabilities);
NsResult ns_app_reload(NsApplication*);
NsResult ns_app_export_history(NsApplication*, uint32_t now_ms);
NsResult ns_app_start(NsApplication*, NsRadio, bool ble, bool selector_confirmed, uint32_t now_ms);
NsResult ns_app_cc_tune(NsApplication*, uint32_t frequency_hz, uint32_t now_ms);
NsResult ns_app_device_info(NsApplication*, uint32_t now_ms);
void ns_app_baseline(NsApplication*);
NsResult ns_app_stop(NsApplication*, uint32_t now_ms);
NsResult ns_app_save_settings(NsApplication*, const NsSettings*);
NsResult ns_app_step(NsApplication*, uint32_t now_ms);
NsResult ns_app_feed(NsApplication*, const uint8_t*, size_t, uint32_t now_ms);
NsResult ns_app_key(NsApplication*, NsKey, uint32_t now_ms);
void ns_app_frame(const NsApplication*, NsFrame*);
const char* ns_result_name(NsResult);
#endif
