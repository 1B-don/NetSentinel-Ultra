#ifndef NS_SESSION_H
#define NS_SESSION_H
#include "radios.h"
#include "marauder.h"
#include "../../ap_parser.h"
#define NS_AP_LIMIT 48
#define NS_LINE_LIMIT 256
#define NS_HISTORY_LIMIT 16
#define NS_CC_SAMPLE_MS 100
#define NS_CC_THRESHOLD_DBM (-80)
/* A profile may be enabled only after installed firmware is verified to issue
 * no over-air TX, including Wi-Fi probe requests and active BLE scan requests.
 * Commands are deliberately absent from the default profile. */
typedef struct {
    const char* firmware;
    const char* wifi_receive;
    const char* ble_receive;
    const char* stop;
    bool verified_receive_only;
} NsEspProfile;
typedef struct { uint32_t id, started_ms, ended_ms, observations; NsRadio radio; NsResult result; } NsHistory;
typedef struct {
    const NsPlatform* io;
    const NsEspProfile* profile;
    NsRadio active;
    NsMarauder marauder;
    bool use_marauder, wifi_request, installed_wifi_verified;
    NsNrf nrf;
    NsCc cc;
    NsAccessPoint aps[NS_AP_LIMIT];
    size_t ap_count;
    int16_t ap_min[NS_AP_LIMIT], ap_max[NS_AP_LIMIT];
    uint32_t ap_seen_ms[NS_AP_LIMIT];
    char line[NS_LINE_LIMIT];
    size_t line_len;
    bool discard_line, ble, logging;
    uint32_t dropped_lines, lines, last_rx_ms, started_ms, id, observations;
    uint32_t timeout_ms, dwell_us;
    uint8_t nrf_channel;
    bool cc_sampled;
    uint32_t cc_last_ms;
    NsResult error;
    NsHistory history[NS_HISTORY_LIMIT];
    size_t history_count, history_next;
} NsSession;
void ns_session_init(NsSession*, const NsPlatform*, const NsEspProfile*);
NsResult ns_session_start(NsSession*, NsRadio, bool ble, bool selector_confirmed, uint32_t now_ms);
NsResult ns_session_cancel(NsSession*, uint32_t now_ms);
void ns_session_stop(NsSession*, uint32_t now_ms);
NsResult ns_session_feed(NsSession*, const uint8_t*, size_t, uint32_t now_ms);
NsResult ns_session_tick(NsSession*, uint32_t now_ms);
NsResult ns_session_cc_tune(NsSession*, uint32_t frequency_hz, uint32_t now_ms);
NsResult ns_session_cc_diagnose(NsSession*, uint32_t now_ms);
NsResult ns_session_log_open(NsSession*, const char* path);
NsResult ns_session_log_close(NsSession*);
#endif
