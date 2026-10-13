#include "../../include/netsentinel/marauder.h"
#include <ctype.h>
#include <string.h>
static NsResult send(NsMarauder* m, const char* text) {
    return m->io->uart_write(m->io->ctx, (const uint8_t*)text, strlen(text));
}
static NsResult fail(NsMarauder* m, NsResult r) { m->stage = NS_M_FAILED; return m->result = r; }
static bool token(const char* line, const char* text) {
    size_t n = strlen(text);
    for(const char* p = line; (p = strstr(p, text)); p++) {
        if((p == line || (!isalnum((unsigned char)p[-1]) && p[-1] != '_')) && !isalnum((unsigned char)p[n]) && p[n] != '_' && p[n] != '.') return true;
    }
    return false;
}
enum { INFO_VERSION=1u, INFO_HW=2u, INFO_IDF=4u, INFO_FW=8u,
    INFO_WSL=16u, INFO_SD=32u, INFO_SIZE=64u, INFO_BATTERY=128u, INFO_ALL=255u };
static NsResult field(NsMarauder* m, char* out, size_t cap, const char* line,
                      const char* label, unsigned bit) {
    size_t n = strlen(label); if(strncmp(line, label, n)) return NS_OK;
    const char* p = line+n; while(*p == ' ') p++;
    size_t len = strlen(p); while(len && p[len-1] == ' ') len--;
    if(!len || len >= cap || (m->info_fields & bit)) return NS_INVALID;
    for(size_t i = 0; i < len; i++)
        if(!isalnum((unsigned char)p[i]) && p[i] != ' ' && p[i] != '.' && p[i] != '-' && p[i] != '_') return NS_INVALID;
    memcpy(out, p, len); out[len] = 0; m->info_fields |= bit;
    return NS_OK;
}
static NsResult info_line(NsMarauder* m, const char* line) {
    NsResult r = field(m, m->firmware, sizeof(m->firmware), line, "Firmware:", INFO_FW);
    if(r == NS_OK) r = field(m, m->version, sizeof(m->version), line, "Version:", INFO_VERSION);
    if(r == NS_OK) r = field(m, m->hardware, sizeof(m->hardware), line, "Hardware:", INFO_HW);
    if(r == NS_OK) r = field(m, m->idf, sizeof(m->idf), line, "ESP-IDF:", INFO_IDF);
    if(r == NS_OK) r = field(m, m->idf, sizeof(m->idf), line, "ESP-IDF Version:", INFO_IDF);
    if(r != NS_OK) return r;
    char value[24] = {0};
    unsigned bit = 0;
    if(!strncmp(line, "WSL Bypass:", 11)) bit = INFO_WSL;
    else if(!strncmp(line, "SD Card:", 8)) bit = INFO_SD;
    else if(!strncmp(line, "SD Card Size:", 13)) bit = INFO_SIZE;
    else if(!strncmp(line, "Battery Monitor:", 16)) bit = INFO_BATTERY;
    if(!bit) return NS_OK; /* unknown fields are never retained or logged */
    const char* label = bit == INFO_WSL ? "WSL Bypass:" : bit == INFO_SD ? "SD Card:" :
        bit == INFO_SIZE ? "SD Card Size:" : "Battery Monitor:";
    r = field(m, value, sizeof(value), line, label, bit);
    if(r != NS_OK) return r;
    if(bit == INFO_WSL) {
        if(strcmp(value, "enabled") && strcmp(value, "disabled")) return NS_INVALID;
        m->wsl_bypass = !strcmp(value, "enabled");
    } else if(bit == INFO_SD) {
        if(strcmp(value, "Connected") && strcmp(value, "Not Connected")) return NS_INVALID;
        m->sd_connected = !strcmp(value, "Connected");
    } else if(bit == INFO_BATTERY) {
        if(strcmp(value, "not supported")) return NS_INVALID;
        m->battery_unsupported = true;
    } else {
        uint32_t size = 0; const char* p = value;
        if(!isdigit((unsigned char)*p)) return NS_INVALID;
        while(isdigit((unsigned char)*p)) {
            unsigned digit = (unsigned)(*p++ - '0');
            if(size > (UINT32_MAX-digit)/10) return NS_INVALID;
            size = size*10 + digit;
        }
        if(size ? strcmp(p, "MB") != 0 : *p != 0) return NS_INVALID;
        m->sd_size_mb = size;
    }
    return NS_OK;
}
NsResult ns_marauder_begin(NsMarauder* m, const NsPlatform* io, bool wifi, bool verified, uint32_t timeout, uint32_t now) {
    if(!m || !io || !io->uart_write || timeout < 1000 || timeout > 60000) return NS_INVALID;
#ifdef NS_ESP_INFO_ONLY
    if(wifi) return NS_UNSUPPORTED;
#elif !defined(NS_MARAUDER_PASSIVE_START_VERIFIED) && !defined(NS_TEST_MARAUDER_PROTOCOL_SIM)
    /* The reviewed v1.11.0 sniffbeacon CLI path first runs initWiFi(), which
     * starts a hidden AP before RunBeaconScan switches to NULL mode. It also
     * opens PCAP output (SavePCAP defaults true). Version matching alone is
     * not evidence that this path is receive-only or storage-safe. */
    if(wifi) return NS_UNSUPPORTED;
#endif
    if(wifi && !verified) return NS_UNSUPPORTED;
    memset(m, 0, sizeof(*m)); m->io = io; m->wifi = wifi; m->installed_verified = verified;
    m->timeout_ms = timeout; m->stage_ms = now; m->stage = NS_M_INFO;
    NsResult r = send(m, "info\n"); return r == NS_OK ? r : fail(m, r);
}
NsResult ns_marauder_line(NsMarauder* m, const char* line, uint32_t now) {
    if(!m || !line) return NS_INVALID;
    NsResult deadline = ns_marauder_tick(m, now);
    if(deadline != NS_OK) return deadline;
    while(*line == ' ') line++;
    const char* echo = m->stage == NS_M_INFO ? "#info" : m->stage == NS_M_HELP ? "#help" :
        m->stage == NS_M_START ? "#sniffbeacon" : m->stage == NS_M_STOP ? "#stopscan" : NULL;
    if(echo && !strcmp(line, echo)) { m->command_echo = true; return NS_OK; }
    if(echo && !m->command_echo) return NS_OK; /* ignore stale/startup chatter */
    if(m->stage == NS_M_INFO) {
        NsResult r = info_line(m, line);
        if(r != NS_OK) return fail(m, r);
    } else if(m->stage == NS_M_HELP) {
        m->has_beacon |= token(line, "sniffbeacon"); m->has_stop |= token(line, "stopscan");
    } else if(m->stage == NS_M_START) {
        /* v1.11.0 startScanFromCLI concatenates Starting without a space. */
        m->ack |= !strcmp(line, "StartingBeacon sniff. Stop with stopscan");
    }
    if(m->stage == NS_M_STOP && !strcmp(line, "Stopping WiFi tran/recv")) m->ack = true;
    if((m->stage == NS_M_HELP || m->stage == NS_M_START) &&
       (!strncmp(line, "Unknown command", 15) || !strncmp(line, "Command not found", 17))) return fail(m, NS_UNSUPPORTED);
    return NS_OK;
}
NsResult ns_marauder_prompt(NsMarauder* m, uint32_t now) {
    if(!m) return NS_INVALID;
    NsResult deadline = ns_marauder_tick(m, now);
    if(deadline != NS_OK) return deadline;
    NsResult r = NS_OK;
    if(m->stage != NS_M_SCAN && !m->command_echo) return NS_OK;
    m->command_echo = false;
    if(m->stage == NS_M_INFO) {
        if(!m->version[0] || !m->hardware[0] || !m->idf[0]) return fail(m, NS_INVALID);
        if((m->info_fields & (INFO_SD | INFO_SIZE)) == (INFO_SD | INFO_SIZE) &&
           m->sd_connected != (m->sd_size_mb > 0)) return fail(m, NS_INVALID);
        m->info_complete = m->info_fields == INFO_ALL && !strcmp(m->firmware, "Marauder");
        m->identity_matches = m->info_complete && !strcmp(m->version, "v1.11.0") &&
            !strcmp(m->hardware, "Marauder v4") && !strcmp(m->idf, "v5.5.1-710-g8410210c9a");
        if(!m->wifi) {
            if(!m->info_complete) return fail(m, NS_INVALID);
            if(!m->identity_matches) return fail(m, NS_UNSUPPORTED);
            m->stage = NS_M_DONE; return NS_OK;
        }
        if(strcmp(m->version, "v1.11.0") || strcmp(m->hardware, "Marauder v4") ||
           strcmp(m->idf, "v5.5.1-710-g8410210c9a")) return fail(m, NS_UNSUPPORTED);
        if(!m->info_complete) return fail(m, NS_INVALID);
        m->stage = NS_M_HELP; r = send(m, "help\n");
    } else if(m->stage == NS_M_HELP) {
        if(!m->has_beacon || !m->has_stop || !m->installed_verified) return fail(m, NS_UNSUPPORTED);
        m->stage = NS_M_START; m->scan_sent = true; r = send(m, "sniffbeacon\n");
    } else if(m->stage == NS_M_START) {
        if(!m->ack) return fail(m, NS_INVALID);
        m->stage = NS_M_SCAN; m->last_ap_ms = now;
    } else if(m->stage == NS_M_STOP) {
        if(!m->ack) return fail(m, NS_INVALID);
        m->stop_confirmed = true; m->stage = NS_M_DONE;
    }
    m->stage_ms = now;
    return r == NS_OK ? r : fail(m, r);
}
NsResult ns_marauder_tick(NsMarauder* m, uint32_t now) {
    if(!m) return NS_INVALID;
    if(m->stage == NS_M_FAILED) return m->result;
    if(m->stage == NS_M_SCAN) {
        if((uint32_t)(now-m->last_ap_ms) >= m->timeout_ms) return fail(m, NS_TIMEOUT);
    } else if(m->stage != NS_M_DONE && m->stage != NS_M_IDLE && (uint32_t)(now-m->stage_ms) >= m->timeout_ms) return fail(m, NS_TIMEOUT);
    return NS_OK;
}
NsResult ns_marauder_cancel(NsMarauder* m, uint32_t now) {
    if(!m) return NS_INVALID;
    if(m->stage == NS_M_DONE) return NS_OK;
    if(m->stage == NS_M_STOP) return NS_BUSY;
    if(!m->scan_sent) { m->stage = NS_M_DONE; return NS_OK; }
    m->stop_sent = true; m->stage = NS_M_STOP; m->stage_ms = now; m->command_echo = false; m->ack = false;
    NsResult r = send(m, "stopscan\n");
    return r == NS_OK ? NS_BUSY : fail(m, r);
}
NsResult ns_marauder_abort(NsMarauder* m) {
    if(m && m->scan_sent && !m->stop_sent && m->stage != NS_M_DONE) {
        m->stop_sent = true; return send(m, "stopscan\n");
    }
    return NS_OK;
}
