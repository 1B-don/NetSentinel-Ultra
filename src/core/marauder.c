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
static void field(char* out, size_t cap, const char* line, const char* label) {
    size_t n = strlen(label); if(strncmp(line, label, n)) return;
    const char* p = line+n; while(*p == ' ') p++;
    size_t len = strlen(p); while(len && p[len-1] == ' ') len--;
    if(len >= cap) return; /* truncated identity must not pass verification */
    memcpy(out, p, len); out[len] = 0;
}
NsResult ns_marauder_begin(NsMarauder* m, const NsPlatform* io, bool wifi, bool verified, uint32_t timeout, uint32_t now) {
    if(!m || !io || !io->uart_write || timeout < 1000 || timeout > 60000) return NS_INVALID;
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
        field(m->version, sizeof(m->version), line, "Version:");
        field(m->hardware, sizeof(m->hardware), line, "Hardware:");
        field(m->idf, sizeof(m->idf), line, "ESP-IDF:");
        field(m->idf, sizeof(m->idf), line, "ESP-IDF Version:");
    } else if(m->stage == NS_M_HELP) {
        m->has_beacon |= token(line, "sniffbeacon"); m->has_stop |= token(line, "stopscan");
    } else if(m->stage == NS_M_START) {
        /* v1.11.0 startScanFromCLI concatenates Starting without a space. */
        m->ack |= !strcmp(line, "StartingBeacon sniff. Stop with stopscan");
    }
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
        if(!m->wifi) { m->stage = NS_M_DONE; return NS_OK; }
        if(strcmp(m->version, "v1.11.0") || strcmp(m->hardware, "Marauder v4") ||
           strcmp(m->idf, "v5.5.1-710-g840210c9a")) return fail(m, NS_UNSUPPORTED);
        m->stage = NS_M_HELP; r = send(m, "help\n");
    } else if(m->stage == NS_M_HELP) {
        if(!m->has_beacon || !m->has_stop || !m->installed_verified) return fail(m, NS_UNSUPPORTED);
        m->stage = NS_M_START; m->scan_sent = true; r = send(m, "sniffbeacon\n");
    } else if(m->stage == NS_M_START) {
        if(!m->ack) return fail(m, NS_INVALID);
        m->stage = NS_M_SCAN; m->last_ap_ms = now;
    } else if(m->stage == NS_M_STOP) m->stage = NS_M_DONE;
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
    m->stop_sent = true; m->stage = NS_M_STOP; m->stage_ms = now; m->command_echo = false;
    NsResult r = send(m, "stopscan\n");
    return r == NS_OK ? NS_BUSY : fail(m, r);
}
NsResult ns_marauder_abort(NsMarauder* m) {
    if(m && m->scan_sent && !m->stop_sent && m->stage != NS_M_DONE) {
        m->stop_sent = true; return send(m, "stopscan\n");
    }
    return NS_OK;
}
