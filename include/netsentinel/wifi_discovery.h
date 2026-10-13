#ifndef NS_WIFI_DISCOVERY_H
#define NS_WIFI_DISCOVERY_H
#include "platform.h"
#include "../../ap_parser.h"
#define NS_WIFI_RESULTS 48u
#define NS_WIFI_LINE_BYTES 192u
#define NS_WIFI_REPLY_BYTES 16384u
typedef enum { NS_WIFI_SECURITY_UNREPORTED, NS_WIFI_SECURITY_OPEN, NS_WIFI_SECURITY_WEP,
    NS_WIFI_SECURITY_WPA, NS_WIFI_SECURITY_WPA2, NS_WIFI_SECURITY_WPA3,
    NS_WIFI_SECURITY_MIXED, NS_WIFI_SECURITY_WPA2_ENTERPRISE,
    NS_WIFI_SECURITY_WPA3_ENTERPRISE, NS_WIFI_SECURITY_WAPI, NS_WIFI_SECURITY_UNKNOWN } NsWifiSecurity;
typedef struct { NsAccessPoint ap; uint16_t index; NsWifiSecurity security; bool bssid_reported; } NsWifiRecord;
typedef enum { NS_WIFI_UNAVAILABLE, NS_WIFI_STARTING, NS_WIFI_OBSERVING, NS_WIFI_STOPPING,
    NS_WIFI_READING, NS_WIFI_COMPLETE, NS_WIFI_EMPTY, NS_WIFI_ERROR,
    NS_WIFI_TIMEOUT, NS_WIFI_CANCELLED } NsWifiState;
/* Intent only: no UART callback or physical binding exists. */
typedef enum { NS_WIFI_INTENT_NONE, NS_WIFI_INTENT_START, NS_WIFI_INTENT_STOP, NS_WIFI_INTENT_LIST } NsWifiIntent;
typedef struct {
    NsWifiState state; NsWifiIntent intent; NsResult result;
    uint32_t stage_ms, timeout_ms, dwell_ms, total_bytes;
    bool echo, ack, peer_may_scan, cancel_requested, summary, results_valid;
    char line[NS_WIFI_LINE_BYTES]; size_t line_len; bool pending_cr;
    NsWifiRecord records[NS_WIFI_RESULTS]; size_t count;
} NsWifiDiscovery;
void ns_wifi_discovery_init(NsWifiDiscovery*);
/* Production always fails closed. Host protocol tests explicitly opt in. */
NsResult ns_wifi_discovery_begin(NsWifiDiscovery*, uint32_t now, uint32_t timeout, uint32_t dwell);
NsResult ns_wifi_discovery_line(NsWifiDiscovery*, const char*, uint32_t now);
NsResult ns_wifi_discovery_feed(NsWifiDiscovery*, const uint8_t*, size_t count, uint32_t now);
NsResult ns_wifi_discovery_tick(NsWifiDiscovery*, uint32_t now);
NsResult ns_wifi_discovery_cancel(NsWifiDiscovery*, uint32_t now);
/* Pure parsers: identities are volatile comparison data, never UI/CSV. Outputs
 * unchanged on error. Detail accepts the reviewed info -a response subset. */
NsResult ns_wifi_parse_list(const char*, NsWifiRecord*);
NsResult ns_wifi_parse_detail(const char*, size_t bytes, NsWifiRecord*);
const char* ns_wifi_security_name(NsWifiSecurity);
#endif
