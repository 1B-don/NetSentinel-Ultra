#include "../../include/netsentinel/session.h"
#include "../../ns_analysis.h"
#include <stdio.h>
#include <string.h>
static NsResult cc_metadata(NsSession*, uint32_t now);
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
/* Arbitrary serial text can contain device/network identifiers or secrets.
 * Persist only fixed telemetry; never echo UART bytes into any log. */
static NsResult raw_log(NsSession* s, uint32_t now) {
    char record[80];
    snprintf(record, sizeof(record), "%lu,%lu,uart_redacted,,,1,line\n",
        (unsigned long)s->id, (unsigned long)now);
    return write_log(s, record);
}
void ns_session_init(NsSession* s, const NsPlatform* io, const NsEspProfile* p) {
    memset(s, 0, sizeof(*s)); s->io = io; s->profile = p; s->timeout_ms = 5000; s->dwell_us = 200;
}
NsResult ns_session_log_close(NsSession* s) {
    if(!s || !s->logging) return NS_OK;
    NsResult r = s->io->file_sync(s->io->ctx);
    NsResult closed = s->io->file_close(s->io->ctx); s->logging = false;
    if(r == NS_OK) r = closed;
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
    r = write_log(s, "session,time_ms,kind,label,channel_or_hz,value,unit\n");
    if(r == NS_OK && s->active == NS_RADIO_CC) {
        char record[120];
        snprintf(record, sizeof(record), "%lu,%lu,cc_config,sample_period,%lu,%u,ms\n",
            (unsigned long)s->id, (unsigned long)s->started_ms, (unsigned long)s->cc.frequency, NS_CC_SAMPLE_MS);
        r = write_log(s, record);
        if(r == NS_OK) {
            snprintf(record, sizeof(record), "%lu,%lu,cc_config,activity_threshold,%lu,%d,dBm\n",
                (unsigned long)s->id, (unsigned long)s->started_ms, (unsigned long)s->cc.frequency, NS_CC_THRESHOLD_DBM);
            r = write_log(s, record);
        }
    }
    if(r == NS_OK && s->active == NS_RADIO_CC) r = cc_metadata(s, s->started_ms);
    return r;
}
NsResult ns_session_start(NsSession* s, NsRadio radio, bool ble, bool confirmed, uint32_t now) {
    if(!s || !s->io || !s->io->acquire || !s->io->release) return NS_INVALID;
    if(s->active != NS_RADIO_NONE) return NS_BUSY;
    if(s->id == UINT32_MAX) return NS_FULL;
    NsResult r;
    if(radio == NS_RADIO_NRF) r = ns_nrf_open(&s->nrf, s->io, confirmed);
    else if(radio == NS_RADIO_CC) r = ns_cc_open(&s->cc, s->io, confirmed);
    else if(radio == NS_RADIO_ESP) {
        if(s->use_marauder) {
            if(ble || (s->wifi_request && !s->installed_wifi_verified)) return NS_UNSUPPORTED;
        } else if(!s->profile || !s->profile->verified_receive_only || !s->profile->firmware ||
           !s->profile->stop || !(ble ? s->profile->ble_receive : s->profile->wifi_receive)) return NS_UNSUPPORTED;
        r = s->io->acquire(s->io->ctx, radio);
        if(r == NS_OK) {
            r = s->use_marauder ? ns_marauder_begin(&s->marauder, s->io, s->wifi_request,
                s->installed_wifi_verified, s->timeout_ms, now) :
                command(s, ble ? s->profile->ble_receive : s->profile->wifi_receive);
            if(r != NS_OK) s->io->release(s->io->ctx, radio);
        }
    } else return NS_INVALID;
    if(r != NS_OK) { s->error = r; return r; }
    s->active = radio; s->ble = ble; s->started_ms = s->last_rx_ms = now;
    s->line_len = 0; s->discard_line = false; s->lines = s->dropped_lines = 0;
    s->ap_count = s->observations = 0; s->nrf_channel = 0; s->cc_sampled = false; s->error = NS_OK; s->id++;
    return NS_OK;
}
void ns_session_stop(NsSession* s, uint32_t now) {
    if(!s || s->active == NS_RADIO_NONE) return;
    NsResult cleanup = NS_OK;
    if(s->active == NS_RADIO_NRF) cleanup = ns_nrf_close(&s->nrf);
    if(s->active == NS_RADIO_CC) cleanup = ns_cc_close(&s->cc);
    if(cleanup != NS_OK && s->error == NS_OK) s->error = cleanup;
    if(s->active == NS_RADIO_ESP) {
        NsResult r = NS_OK;
        if(s->use_marauder) r = ns_marauder_abort(&s->marauder);
        else r = command(s, s->profile->stop);
        if(r != NS_OK && s->error == NS_OK) s->error = r;
        s->io->release(s->io->ctx, NS_RADIO_ESP);
    }
    if(s->active == NS_RADIO_ESP && s->use_marauder && s->ap_count && s->logging) {
        uint8_t channels[14]; ns_channel_counts(s->aps, s->ap_count, channels);
        for(unsigned ch = 0; ch < 14 && s->logging; ch++) {
            char record[100];
            snprintf(record, sizeof(record), "%lu,%lu,wifi_channel,,%u,%u,APs\n",
                (unsigned long)s->id, (unsigned long)now, ch+1, channels[ch]);
            (void)write_log(s, record);
        }
    }
    (void)ns_session_log_close(s);
    s->history[s->history_next] = (NsHistory){s->id, s->started_ms, now, s->observations, s->active, s->error};
    s->history_next = (s->history_next + 1) % NS_HISTORY_LIMIT;
    if(s->history_count < NS_HISTORY_LIMIT) s->history_count++;
    s->active = NS_RADIO_NONE;
}
NsResult ns_session_cancel(NsSession* s, uint32_t now) {
    if(!s) return NS_INVALID;
    if(s->active == NS_RADIO_ESP && s->use_marauder) {
        NsResult r = ns_marauder_cancel(&s->marauder, now);
        if(r == NS_BUSY) return r; /* retain UART until prompt or deadline */
        if(r != NS_OK) s->error = r;
    }
    ns_session_stop(s, now); return s->error;
}
NsResult ns_session_feed(NsSession* s, const uint8_t* data, size_t count, uint32_t now) {
    if(!s || s->active != NS_RADIO_ESP || (!data && count)) return NS_INVALID;
    NsResult result = NS_OK;
    for(size_t i = 0; i < count; i++) {
        unsigned char c = data[i];
        if(c == '\r') continue;
        if(s->use_marauder && !s->discard_line && c == ' ' && s->line_len == 1 && s->line[0] == '>') {
            NsResult r = ns_marauder_prompt(&s->marauder, now);
            s->line_len = 0;
            if(r != NS_OK || s->marauder.stage == NS_M_DONE) {
                s->error = r; ns_session_stop(s, now); return r;
            }
            continue;
        }
        if(c == '\n') {
            if(s->discard_line) {
                s->dropped_lines++;
                if(s->use_marauder && s->marauder.stage != NS_M_SCAN) {
                    s->error = NS_FULL; ns_session_stop(s, now); return NS_FULL;
                }
            }
            else if(s->line_len) {
                s->line[s->line_len] = 0;
                s->lines++; s->last_rx_ms = now;
                NsResult logged = raw_log(s, now);
                if(logged != NS_OK) {
                    s->error = logged; ns_session_stop(s, now); return logged;
                }
                if(s->use_marauder) {
                    NsResult r = ns_marauder_line(&s->marauder, s->line, now);
                    if(r != NS_OK) { s->error = r; ns_session_stop(s, now); return r; }
                }
                NsAccessPoint ap;
                if(!s->ble && (!s->use_marauder || s->marauder.stage == NS_M_SCAN) && ns_parse_ap_line(s->line, &ap)) {
                    if(s->use_marauder) s->marauder.last_ap_ms = now;
                    size_t index = 0;
                    for(; index < s->ap_count; index++) {
                        NsAccessPoint* old = &s->aps[index];
                        if(ap.bssid[0] && old->bssid[0] ? !strcmp(ap.bssid, old->bssid) :
                           ap.channel == old->channel && !strcmp(ap.ssid, old->ssid)) break;
                    }
                    if(index < NS_AP_LIMIT) {
                        if(index == s->ap_count) {
                            s->ap_min[index] = s->ap_max[index] = ap.rssi; s->ap_count++;
                        } else {
                            if(ap.rssi < s->ap_min[index]) s->ap_min[index] = ap.rssi;
                            if(ap.rssi > s->ap_max[index]) s->ap_max[index] = ap.rssi;
                        }
                        s->ap_seen_ms[index] = now; s->aps[index] = ap;
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
static NsResult cc_metadata(NsSession* s, uint32_t now) {
    if(!s->cc.diagnostic_valid) return NS_OK;
    const NsCcDiagnostics* d = &s->cc.diagnostic;
    const char* labels[] = {"part", "version", "state", "rssi_raw", "spi_transactions"};
    const uint32_t values[] = {d->part, d->version, d->state, d->rssi_raw, d->transactions};
    for(unsigned i = 0; i < 5; i++) {
        char record[120];
        snprintf(record, sizeof(record), "%lu,%lu,cc_diag,%s,%lu,%lu,%s\n", (unsigned long)s->id,
            (unsigned long)now, labels[i], (unsigned long)s->cc.frequency, (unsigned long)values[i], i == 4 ? "count" : "byte");
        NsResult r = write_log(s, record); if(r != NS_OK) return r;
    }
    return NS_OK;
}
NsResult ns_session_cc_diagnose(NsSession* s, uint32_t now) {
    if(!s || s->active != NS_RADIO_CC) return NS_INVALID;
    NsResult r = ns_cc_diagnose(&s->cc);
    if(r == NS_UNSUPPORTED) return r;
    if(r == NS_OK && s->cc.frequency && s->cc.diagnostic.state != 0x0d) r = NS_IO;
    if(r == NS_OK) r = cc_metadata(s, now);
    if(r != NS_OK) { s->error = r; ns_session_stop(s, now); }
    return r;
}
NsResult ns_session_cc_tune(NsSession* s, uint32_t frequency, uint32_t now) {
    if(!s || s->active != NS_RADIO_CC || !ns_cc_frequency_valid(frequency)) return NS_INVALID;
    NsResult r = ns_cc_tune(&s->cc, frequency);
    if(r == NS_OK) {
        s->cc_sampled = false;
        char record[100];
        snprintf(record, sizeof(record), "%lu,%lu,cc_tune,,%lu,,Hz\n", (unsigned long)s->id,
            (unsigned long)now, (unsigned long)frequency);
        r = write_log(s, record);
        if(r == NS_OK) r = cc_metadata(s, now);
    }
    if(r != NS_OK) { s->error = r; ns_session_stop(s, now); }
    return r;
}
NsResult ns_session_tick(NsSession* s, uint32_t now) {
    if(!s || s->active == NS_RADIO_NONE) return NS_INVALID;
    NsResult r = NS_OK;
    char record[120];
    if(s->active == NS_RADIO_ESP) {
        if(s->use_marauder) r = ns_marauder_tick(&s->marauder, now);
        else if((uint32_t)(now - s->last_rx_ms) >= s->timeout_ms) r = NS_TIMEOUT;
    } else if(s->active == NS_RADIO_NRF) {
        uint8_t ch = s->nrf_channel;
        r = ns_nrf_sample(&s->nrf, ch, s->dwell_us);
        if(r == NS_OK) {
            s->observations++;
            snprintf(record, sizeof(record), "%lu,%lu,nrf,,%u,%u,percent\n",
                (unsigned long)s->id, (unsigned long)now, ch, ns_nrf_percent(&s->nrf, ch));
            r = write_log(s, record);
            s->nrf_channel = (ch + 1) % NS_NRF_CHANNELS;
        }
    } else {
        if(s->cc_sampled && (uint32_t)(now-s->cc_last_ms) < NS_CC_SAMPLE_MS) return NS_OK;
        r = ns_cc_sample(&s->cc);
        if(r == NS_OK) {
            s->cc_sampled = true; s->cc_last_ms = now; s->observations++;
            snprintf(record, sizeof(record), "%lu,%lu,cc,,%lu,%d,dBm\n",
                (unsigned long)s->id, (unsigned long)now, (unsigned long)s->cc.frequency, s->cc.last);
            r = write_log(s, record);
            if(r == NS_OK) {
                snprintf(record, sizeof(record), "%lu,%lu,cc_activity,above_%d,%lu,%u,percent\n",
                    (unsigned long)s->id, (unsigned long)now, NS_CC_THRESHOLD_DBM, (unsigned long)s->cc.frequency,
                    ns_cc_above_percent(&s->cc, NS_CC_THRESHOLD_DBM));
                r = write_log(s, record);
            }
        }
    }
    if(r != NS_OK) { s->error = r; ns_session_stop(s, now); }
    return r;
}
