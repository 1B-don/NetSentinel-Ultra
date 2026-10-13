#ifndef NS_APPLICATION_H
#define NS_APPLICATION_H
#include "observation.h"
#include "simulation.h"
typedef enum { NS_KEY_UP, NS_KEY_DOWN, NS_KEY_LEFT, NS_KEY_RIGHT, NS_KEY_OK, NS_KEY_BACK } NsKey;
typedef enum { NS_PAGE_HOME, NS_PAGE_RADIO, NS_PAGE_SELECTOR, NS_PAGE_SETTINGS, NS_PAGE_HISTORY, NS_PAGE_DIAGNOSTICS, NS_PAGE_FILTERS, NS_PAGE_EXPORTS, NS_PAGE_WIFI_OVERVIEW, NS_PAGE_MODES, NS_PAGE_SIMULATION } NsPage;
#define NS_MENU_COUNT 11u
typedef enum { NS_MODE_MONITOR, NS_MODE_LAB, NS_MODE_SIMULATION, NS_MODE_HARDWARE_DIAGNOSTICS, NS_MODE_COUNT } NsOperatingMode;
typedef struct { bool nrf, cc, esp_uart, esp_wifi_verified, esp_info_only; } NsCapabilities;
typedef enum { NS_ESP_DISCONNECTED, NS_ESP_CONNECTING, NS_ESP_CONNECTED,
    NS_ESP_TIMEOUT, NS_ESP_PROTOCOL_ERROR, NS_ESP_CANCELLED } NsEspConnectionState;
#define NS_ESP_INFO_VIEWS 4u
typedef enum { NS_CAP_SUPPORTED, NS_CAP_SIMULATED, NS_CAP_UNAVAILABLE, NS_CAP_UNVERIFIED } NsCapabilityState;
typedef enum { NS_IF_NONE, NS_IF_UART, NS_IF_SPI, NS_IF_SUBGHZ_REGISTRY } NsCapabilityInterface;
enum {
    NS_CAP_BLOCK_ROUTE = 1u << 0,
    NS_CAP_BLOCK_PROVIDER = 1u << 1,
    NS_CAP_BLOCK_PASSIVE_POLICY = 1u << 2,
    NS_CAP_BLOCK_PEER_STORAGE = 1u << 3,
    NS_CAP_BLOCK_ACTIVE_BLE = 1u << 4,
};
/* Describes software evidence separately from physical readiness. `available`
 * never means that a guessed board binding may be opened. */
typedef struct {
    NsCapabilityState state;
    NsCapabilityInterface interface;
    bool interface_available;
    bool routing_verified;
    bool receiver_supported;
    bool simulation_available;
    bool physical_enabled;
    uint8_t blockers;
} NsRadioCapability;
/* Wi-Fi requires installed-build command/output verification in addition to
 * a UART port. BLE is unsupported. Information queries do not scan. */
typedef struct {
    NsSession session;
    NsSavedState saved;
    NsPersistence persistence;
    NsCapabilities capabilities;
    NsRadioCapability esp_info_capability, wifi_capability, ble_capability;
    NsRadioCapability nrf_capability, cc_capability;
    /* Volatile by design: restored settings can never enable a mode or radio. */
    NsOperatingMode mode;
    NsSimulation simulation;
    NsPage page;
    unsigned menu, setting, history_index, filter_setting, diagnostic_view, mode_selection, simulation_action;
    NsObservationFilter observation_filter;
    NsHistoryFilter history_filter;
    uint32_t now_ms, export_id;
    NsRadio selected;
    bool ble, running, history_dirty, exit_requested, wifi_details, cc_details, nrf_details;
    unsigned ap_index, info_pending, info_view;
    /* Volatile bounded diagnostics; never serialized or copied from raw UART. */
    bool info_timing, info_validated, info_cancelled, info_sd_connected;
    uint32_t info_elapsed_ms, info_success_id;
    /* Latest info outcome is independent of unrelated UI/storage actions. */
    NsEspConnectionState info_state;
    NsResult info_result;
    bool info_attempted, info_reply_complete;
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
