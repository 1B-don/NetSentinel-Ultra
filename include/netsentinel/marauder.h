#ifndef NS_MARAUDER_H
#define NS_MARAUDER_H
#include "platform.h"
#include "../../ap_parser.h"
typedef enum { NS_M_IDLE, NS_M_INFO, NS_M_HELP, NS_M_START, NS_M_SCAN, NS_M_STOP, NS_M_DONE, NS_M_FAILED } NsMarauderStage;
typedef struct {
    const NsPlatform* io;
    NsMarauderStage stage;
    uint32_t stage_ms, last_ap_ms, timeout_ms;
    bool wifi, installed_verified, scan_sent, stop_sent, ack, has_beacon, has_stop, command_echo;
    char firmware[24], version[24], hardware[40], idf[64];
    unsigned info_fields;
    uint32_t sd_size_mb;
    bool sd_connected, battery_unsupported, wsl_bypass;
    bool info_complete, identity_matches; /* only set after a complete framed reply */
    NsResult result;
} NsMarauder;
NsResult ns_marauder_begin(NsMarauder*, const NsPlatform*, bool wifi, bool installed_verified, uint32_t timeout_ms, uint32_t now_ms);
NsResult ns_marauder_line(NsMarauder*, const char*, uint32_t now_ms);
NsResult ns_marauder_prompt(NsMarauder*, uint32_t now_ms);
NsResult ns_marauder_tick(NsMarauder*, uint32_t now_ms);
NsResult ns_marauder_cancel(NsMarauder*, uint32_t now_ms);
NsResult ns_marauder_abort(NsMarauder*);
#endif
