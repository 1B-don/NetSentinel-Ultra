#include "netsentinel/session.h"
#include "ns_analysis.h"
#include <stdio.h>
#include <string.h>
static NsResult command(NsSession* s, const char* text) {
    if(!text || !*text || strlen(text) > 80 || !s->io->uart_write) return NS_UNSUPPORTED;
    return s->io->uart_write(s->io->ctx, (const uint8_t*)text, strlen(text));
}
static NsResult write_log(NsSession* s, const char* text) {
    if(!s->logging) return NS_OK;
    NsResult r = s->io->file_write(s->io->ctx, text, strlen(text));
    if(r != NS_OK) { s->io->file_close(s->io->ctx); s->logging = false; s->error = r; }
    return r;
}
/* Use quoted bounded raw text so BLE formats can be logged without inventing
 * device fields. Unknown BLE lines remain raw observations, never device counts. */
static NsResult raw_log(NsSession* s, uint32_t now) {
    char record[2 * NS_LINE_LIMIT + 80];
    int len = snprintf(record, sizeof(record), "%lu,%lu,%s,\"", (unsigned long)s->id,
                       (unsigned long)now, s->ble ? "ble_raw" : "uart_raw");
    size_t pos = (size_t)len;
    for(size_t i = 0; i < s->line_len; i++) {
        char c = s->line[i];
        if(c == '"') record[pos++] = '"';
        record[pos++] = c;
    }
    memcpy(record + pos, "\",,,\n", 6);
    return write_log(s, record);
}
void ns_session_init(NsSession* s, const NsPlatform* io, const NsEspProfile* p) {
    memset(s, 0, sizeof(*s)); s->io = io; s->profile = p; s->timeout_ms = 5000;
}
NsResult ns_session_log_close(NsSession* s) {
    if(!s || !s->logging) return NS_OK;
    NsResult r = s->io->file_sync(s->io->ctx);
    s->io->file_close(s->io->ctx); s->logging = false;
    if(r != NS_OK) s->error = r;
    return r;
}
NsResult ns_session_log_open(NsSession* s, const char* path) {
    if(!s || !s->io || !path || s->active == NS_RADIO_NONE || s->logging || !s->io->file_create || !s->io->file_write || !s->io->file_sync || !s->io->file_close) return NS_INVALID;
    /* Only fixed SD application directory, no traversal or embedded separators. */
    const char* prefix = "/ext/apps_data/netsentinel/";
    size_t n = strlen(prefix);
    if(strncmp(path, prefix, n) || !path[n] || strstr(path + n, "..") || strchr(path + n, '/') || strchr(path + n, '\\')) return NS_INVALID;
    NsResult r = s->io->file_create(s->io->ctx, path);
    if(r != NS_OK) return r;
    s->logging = true;
    return write_log(s, "session,time_ms,kind,label,channel_or_hz,value,unit\n");
}
NsResult ns_session_start(NsSession* s, NsRadio radio, bool ble, bool confirmed, uint32_t now) {
    if(!s || !s->io || !s->io->acquire || !s->io->release) return NS_INVALID;
    if(s->active != NS_RADIO_NONE) return NS_BUSY;
    if(s->id == UINT32_MAX) return NS_FULL;
    NsResult r;
    if(radio == NS_RADIO_NRF) r = ns_nrf_open(&s->nrf, s->io, confirmed);
    else if(radio == NS_RADIO_CC) r = ns_cc_open(&s->cc, s->io, confirmed);
    else if(radio == NS_RADIO_ESP) {
        if(!s->profile || !s->profile->verified_receive_only || !s->profile->firmware ||
           !s->profile->stop || !(ble ? s->profile->ble_receive : s->profile->wifi_receive)) return NS_UNSUPPORTED;
        r = s->io->acquire(s->io->ctx, radio);
        if(r == NS_OK) {
            r = command(s, ble ? s->profile->ble_receive : s->profile->wifi_receive);
            if(r != NS_OK) s->io->release(s->io->ctx, radio);
        }
    } else return NS_INVALID;
    if(r != NS_OK) { s->error = r; return r; }
    s->active = radio; s->ble = ble; s->started_ms = s->last_rx_ms = now;
    s->line_len = 0; s->discard_line = false; s->lines = s->dropped_lines = 0;
    s->ap_count = s->observations = 0; s->nrf_channel = 0; s->error = NS_OK; s->id++;
    return NS_OK;
}
void ns_session_stop(NsSession* s, uint32_t now) {
    if(!s || s->active == NS_RADIO_NONE) return;
    if(s->active == NS_RADIO_NRF) ns_nrf_close(&s->nrf);
    if(s->active == NS_RADIO_CC) ns_cc_close(&s->cc);
    if(s->active == NS_RADIO_ESP) {
        NsResult r = command(s, s->profile->stop);
        if(r != NS_OK && s->error == NS_OK) s->error = r;
        s->io->release(s->io->ctx, NS_RADIO_ESP);
    }
    (void)ns_session_log_close(s);
    s->history[s->history_next] = (NsHistory){s->id, s->started_ms, now, s->observations, s->active, s->error};
    s->history_next = (s->history_next + 1) % NS_HISTORY_LIMIT;
    if(s->history_count < NS_HISTORY_LIMIT) s->history_count++;
    s->active = NS_RADIO_NONE;
}
NsResult ns_session_feed(NsSession* s, const uint8_t* data, size_t count, uint32_t now) {
    if(!s || s->active != NS_RADIO_ESP || (!data && count)) return NS_INVALID;
    NsResult result = NS_OK;
    for(size_t i = 0; i < count; i++) {
        unsigned char c = data[i];
        if(c == '\r') continue;
        if(c == '\n') {
            if(s->discard_line) s->dropped_lines++;
            else if(s->line_len) {
                s->line[s->line_len] = 0;
                s->lines++; s->last_rx_ms = now;
                NsResult logged = raw_log(s, now);
                if(logged != NS_OK) {
                    s->error = logged; ns_session_stop(s, now); return logged;
                }
                NsAccessPoint ap;
                if(!s->ble && ns_parse_ap_line(s->line, &ap)) {
                    size_t index = 0;
                    for(; index < s->ap_count; index++) {
                        NsAccessPoint* old = &s->aps[index];
                        if(ap.bssid[0] && old->bssid[0] ? !strcmp(ap.bssid, old->bssid) :
                           ap.channel == old->channel && !strcmp(ap.ssid, old->ssid)) break;
                    }
                    if(index < NS_AP_LIMIT) {
                        s->aps[index] = ap;
                        if(index == s->ap_count) s->ap_count++;
                        s->observations++;
                    } else result = NS_FULL;
                }
            }
            s->line_len = 0; s->discard_line = false;
        } else if(c == 0 || s->line_len + 1 >= sizeof(s->line)) s->discard_line = true;
        else if(!s->discard_line) s->line[s->line_len++] = c >= 32 && c < 127 ? (char)c : '?';
    }
    return result;
}
NsResult ns_session_tick(NsSession* s, uint32_t now) {
    if(!s || s->active == NS_RADIO_NONE) return NS_INVALID;
    NsResult r = NS_OK;
    char record[120];
    if(s->active == NS_RADIO_ESP) {
        if((uint32_t)(now - s->last_rx_ms) >= s->timeout_ms) r = NS_TIMEOUT;
    } else if(s->active == NS_RADIO_NRF) {
        uint8_t ch = s->nrf_channel;
        r = ns_nrf_sample(&s->nrf, ch, 200);
        if(r == NS_OK) {
            s->observations++;
            snprintf(record, sizeof(record), "%lu,%lu,nrf,,%u,%u,percent\n",
                (unsigned long)s->id, (unsigned long)now, ch, ns_nrf_percent(&s->nrf, ch));
            r = write_log(s, record);
            s->nrf_channel = (ch + 1) % NS_NRF_CHANNELS;
        }
    } else {
        r = ns_cc_sample(&s->cc);
        if(r == NS_OK) {
            s->observations++;
            snprintf(record, sizeof(record), "%lu,%lu,cc,,%lu,%d,dBm\n",
                (unsigned long)s->id, (unsigned long)now, (unsigned long)s->cc.frequency, s->cc.last);
            r = write_log(s, record);
        }
    }
    if(r != NS_OK) { s->error = r; ns_session_stop(s, now); }
    return r;
}
