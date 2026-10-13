#include "../../include/netsentinel/application.h"
#include "../../ns_analysis.h"
#include <stdio.h>
#include <string.h>
static const char* const menu[] = {"Wi-Fi", "BLE", "NRF24 activity", "CC1101 RSSI", "Settings", "History", "Diagnostics", "Filters", "CSV exports"};
const char* ns_result_name(NsResult r) {
    static const char* const names[] = {"OK", "Invalid", "Busy", "Unsupported", "I/O failure", "Absent", "Timeout", "Full"};
    _Static_assert(sizeof(names)/sizeof(names[0]) == NS_FULL+1, "NsResult names must cover every result");
    /* Handles signed and short unsigned enums, including invalid negatives. */
    unsigned index = (unsigned)r;
    return index < sizeof(names)/sizeof(names[0]) ? names[index] : "Unknown";
}
static void clear_cc_measurements(NsApplication* a) {
    a->session.cc.frequency = 0; a->session.cc.samples = 0;
    a->session.cc.trace_count = a->session.cc.trace_next = 0;
    a->session.cc.diagnostic_valid = false;
}
static void apply_settings(NsApplication* a) {
    a->session.timeout_ms = a->saved.settings.timeout_ms;
    a->session.dwell_us = a->saved.settings.dwell_us;
}
NsResult ns_app_init(NsApplication* a, const NsPlatform* io, const NsStore* store, NsCapabilities caps) {
    if(!a || !io || !store) return NS_INVALID;
    memset(a, 0, sizeof(*a)); a->observation_filter = ns_observation_filter_default(); a->capabilities = caps; a->running = true;
    ns_saved_defaults(&a->saved);
    NsResult r = ns_persistence_load(&a->persistence, store, &a->saved);
    ns_session_init(&a->session, io, NULL);
    a->session.use_marauder = caps.esp_uart;
    a->session.installed_wifi_verified = caps.esp_wifi_verified;
    a->session.id = a->saved.last_id;
    a->session.history_count = a->saved.history_count;
    a->session.history_next = a->saved.history_count % NS_HISTORY_LIMIT;
    memcpy(a->session.history, a->saved.history, sizeof(a->saved.history));
    apply_settings(a); a->status = r == NS_ABSENT ? NS_OK : r;
    return r;
}
NsResult ns_app_reload(NsApplication* a) {
    if(!a) return NS_INVALID;
    if(a->session.active != NS_RADIO_NONE || a->info_pending || a->history_dirty) return a->status = NS_BUSY;
    NsSavedState next; NsPersistence loaded;
    NsResult r = ns_persistence_load(&loaded, a->persistence.io, &next);
    if(r == NS_OK || r == NS_ABSENT) {
        if(a->saved.settings.cc_frequency_hz != next.settings.cc_frequency_hz) clear_cc_measurements(a);
        a->persistence = loaded; a->saved = next; a->session.id = next.last_id;
        a->session.history_count = next.history_count;
        a->session.history_next = next.history_count % NS_HISTORY_LIMIT;
        memcpy(a->session.history, next.history, sizeof(next.history));
        a->history_index = 0; apply_settings(a);
    }
    return a->status = r == NS_ABSENT ? NS_OK : r;
}
static NsResult persist_history(NsApplication* a) {
    if(!a->history_dirty) return NS_OK;
    NsSavedState next = a->saved;
    next.history_count = a->session.history_count;
    size_t first = (a->session.history_next + NS_HISTORY_LIMIT - a->session.history_count) % NS_HISTORY_LIMIT;
    for(size_t i = 0; i < next.history_count; i++) next.history[i] = a->session.history[(first+i) % NS_HISTORY_LIMIT];
    NsResult r = ns_persistence_save(&a->persistence, &next);
    if(r == NS_OK) { a->saved = next; a->history_dirty = false; }
    return r;
}
NsResult ns_app_save_settings(NsApplication* a, const NsSettings* settings) {
    if(!a || !settings) return NS_INVALID;
    if(a->session.active != NS_RADIO_NONE || a->info_pending) return a->status = NS_BUSY;
    NsResult r = persist_history(a);
    if(r != NS_OK) return a->status = r;
    NsSavedState next = a->saved; next.settings = *settings;
    r = ns_persistence_save(&a->persistence, &next);
    if(r == NS_OK) {
        if(a->saved.settings.cc_frequency_hz != next.settings.cc_frequency_hz) clear_cc_measurements(a);
        a->saved = next; apply_settings(a);
    }
    return a->status = r;
}
static NsResult start_session(NsApplication* a, NsRadio radio, bool ble, bool confirmed, uint32_t now) {
    if(!a || !a->running) return NS_INVALID;
    a->now_ms = now;
    if(a->session.active != NS_RADIO_NONE || a->info_pending) return a->status = NS_BUSY;
    if(radio == NS_RADIO_ESP) {
        if(ble || a->saved.settings.baud != 115200 || !a->capabilities.esp_uart || (a->session.wifi_request && !a->capabilities.esp_wifi_verified)) return a->status = NS_UNSUPPORTED;
    } else {
        if(radio != NS_RADIO_NRF && radio != NS_RADIO_CC) return a->status = NS_INVALID;
        if(!(radio == NS_RADIO_NRF ? a->capabilities.nrf : a->capabilities.cc)) return a->status = NS_UNSUPPORTED;
        if(!confirmed) return a->status = NS_INVALID;
    }
    NsResult r = persist_history(a);
    if(r != NS_OK) return a->status = r;
    if(a->saved.last_id == UINT32_MAX) return a->status = NS_FULL;
    NsSavedState reservation = a->saved; reservation.last_id++;
    r = ns_persistence_save(&a->persistence, &reservation);
    if(r != NS_OK) return a->status = r; /* no acquisition if ID cannot be durably reserved */
    a->saved = reservation;
    a->session.id = reservation.last_id - 1;
    r = ns_session_start(&a->session, radio, ble, true, now);
    /* Failed starts still consume an ID; never reuse it after reboot. */
    a->session.id = reservation.last_id;
    if(r != NS_OK) return a->status = r;
    if(radio == NS_RADIO_CC) r = ns_cc_tune(&a->session.cc, a->saved.settings.cc_frequency_hz);
    if(r == NS_OK) {
        char path[88];
        snprintf(path, sizeof(path), "/ext/apps_data/netsentinel/session_%010lu.csv", (unsigned long)a->session.id);
        r = ns_session_log_open(&a->session, path);
    }
    if(r != NS_OK) {
        a->session.error = r;
        (void)ns_app_stop(a, now);
        return a->status = r;
    }
    a->selected = radio; a->ble = ble; a->page = NS_PAGE_RADIO;
    return a->status = NS_OK;
}
NsResult ns_app_start(NsApplication* a, NsRadio radio, bool ble, bool confirmed, uint32_t now) {
    if(!a) return NS_INVALID;
    if(a->session.active != NS_RADIO_NONE || a->info_pending) return a->status = NS_BUSY;
    if(a->capabilities.esp_info_only && radio == NS_RADIO_ESP && !ble) return ns_app_device_info(a, now);
    a->session.wifi_request = radio == NS_RADIO_ESP;
    return start_session(a, radio, ble, confirmed, now);
}
NsResult ns_app_cc_tune(NsApplication* a, uint32_t frequency, uint32_t now) {
    if(!a || !ns_cc_frequency_valid(frequency)) return NS_INVALID;
    if(a->session.active == NS_RADIO_NONE) {
        NsSettings s = a->saved.settings; s.cc_frequency_hz = frequency;
        return ns_app_save_settings(a, &s);
    }
    if(a->session.active != NS_RADIO_CC) return a->status = NS_BUSY;
    NsResult r = ns_session_cc_tune(&a->session, frequency, now);
    if(a->session.active == NS_RADIO_NONE) {
        a->history_dirty = true; NsResult stored = persist_history(a); if(stored != NS_OK) r = stored;
    }
    return a->status = r;
}
static void info_finished(NsApplication* a, uint32_t now) {
    if(!a->capabilities.esp_info_only || !a->info_timing) return;
    a->info_elapsed_ms = now - a->session.started_ms;
    if(!a->info_cancelled && a->session.error == NS_OK &&
       a->session.marauder.info_complete && a->session.marauder.identity_matches) {
        a->info_validated = true;
        a->info_sd_connected = a->session.marauder.sd_connected;
        a->info_success_id = a->session.id;
    }
}
NsResult ns_app_device_info(NsApplication* a, uint32_t now) {
    if(!a) return NS_INVALID;
    if(a->session.active != NS_RADIO_NONE || a->info_pending) return a->status = NS_BUSY;
    a->session.wifi_request = false;
    a->info_timing = false; a->info_cancelled = false;
    NsResult r = start_session(a, NS_RADIO_ESP, false, false, now);
    if(r == NS_OK) { a->info_timing = true; a->info_elapsed_ms = 0; }
    return r;
}
void ns_app_baseline(NsApplication* a) {
    if(!a) return;
    a->baseline_count = a->session.ap_count;
    memcpy(a->baseline, a->session.aps, a->baseline_count*sizeof(*a->baseline));
}
NsResult ns_app_stop(NsApplication* a, uint32_t now) {
    if(!a) return NS_INVALID;
    if(a->info_pending && a->capabilities.esp_info_only) a->info_cancelled = true;
    a->info_pending = 0;
    if(a->session.active == NS_RADIO_NONE && !a->history_dirty) return a->status;
    NsResult session_result = NS_OK;
    if(a->session.active != NS_RADIO_NONE) {
        a->info_cancelled = a->capabilities.esp_info_only;
        NsResult cancelled = ns_session_cancel(&a->session, now);
        info_finished(a, now);
        if(cancelled == NS_BUSY) return a->status = NS_BUSY;
        a->history_dirty = true;
        session_result = a->session.error;
    }
    NsResult r = persist_history(a);
    return a->status = r != NS_OK ? r : session_result;
}
NsResult ns_app_step(NsApplication* a, uint32_t now) {
    if(!a || !a->running) return NS_INVALID;
    if(a->info_pending) {
        if(--a->info_pending) return NS_OK; /* publish connecting before hardware */
        return ns_app_device_info(a, now);
    }
    if(a->session.active == NS_RADIO_NONE) return NS_OK;
    NsResult r = NS_OK;
    if(a->capabilities.esp_info_only) a->info_elapsed_ms = now - a->session.started_ms;
    if(a->session.active == NS_RADIO_ESP && a->session.io->uart_read) {
        uint8_t bytes[128];
        for(unsigned batch = 0; batch < 8 && a->session.active == NS_RADIO_ESP; batch++) {
            size_t count = 0;
            r = a->session.io->uart_read(a->session.io->ctx, bytes, sizeof(bytes), &count);
            if(r != NS_OK || count > sizeof(bytes)) {
                if(r == NS_OK) r = NS_FULL;
                a->session.error = r; ns_session_stop(&a->session, now); break;
            }
            if(!count) break;
            r = ns_session_feed(&a->session, bytes, count, now);
            if(r != NS_OK) break;
        }
    }
    if(r == NS_OK && a->session.active != NS_RADIO_NONE) r = ns_session_tick(&a->session, now);
    if(a->session.active == NS_RADIO_NONE) {
        info_finished(a, now);
        a->history_dirty = true;
        NsResult stored = persist_history(a);
        if(stored != NS_OK) r = stored;
    }
    if(a->exit_requested && a->session.active == NS_RADIO_NONE) a->running = false;
    return a->status = r;
}
NsResult ns_app_feed(NsApplication* a, const uint8_t* bytes, size_t n, uint32_t now) {
    if(!a) return NS_INVALID;
    bool active = a->session.active != NS_RADIO_NONE;
    NsResult r = ns_session_feed(&a->session, bytes, n, now);
    if(active && a->session.active == NS_RADIO_NONE) {
        info_finished(a, now);
        a->history_dirty = true;
        NsResult saved = persist_history(a); if(saved != NS_OK) r = saved;
    }
    return a->status = r;
}
static NsResult change_setting(NsApplication* a, bool increase) {
    NsSettings s = a->saved.settings;
    switch(a->setting) {
    case 0: s.timeout_ms = increase ? (s.timeout_ms <= 59000 ? s.timeout_ms+1000 : 60000) : (s.timeout_ms > 1000 ? s.timeout_ms-1000 : 1000); break;
    case 1: s.dwell_us = increase ? (s.dwell_us <= 99990 ? s.dwell_us+10 : 100000) : (s.dwell_us > 180 ? s.dwell_us-10 : 170); break;
    case 2: {
        uint32_t f = increase ? s.cc_frequency_hz+100000 : s.cc_frequency_hz-100000;
        if(!ns_cc_frequency_valid(f)) return a->status = NS_INVALID;
        s.cc_frequency_hz = f; break;
    }
    default: return NS_INVALID;
    }
    return ns_app_save_settings(a, &s);
}
NsResult ns_app_key(NsApplication* a, NsKey key, uint32_t now) {
    /* All valid keys start at zero; unsigned conversion rejects negatives too. */
    if(!a || !a->running || (unsigned)key > NS_KEY_BACK) return NS_INVALID;
    a->now_ms = now;
    if(key == NS_KEY_BACK) {
        NsResult r = ns_app_stop(a, now);
        if(a->page == NS_PAGE_HOME) {
            if(a->session.active != NS_RADIO_NONE) a->exit_requested = true;
            else a->running = false;
        }
        else a->page = NS_PAGE_HOME;
        return r;
    }
    if(a->page == NS_PAGE_HOME) {
        if(key == NS_KEY_UP && a->menu) a->menu--;
        if(key == NS_KEY_DOWN && a->menu < 8) a->menu++;
        if(key == NS_KEY_OK) {
            a->status = NS_OK;
            if(a->menu < 4) {
                a->selected = a->menu < 2 ? NS_RADIO_ESP : (a->menu == 2 ? NS_RADIO_NRF : NS_RADIO_CC);
                a->ble = a->menu == 1; a->page = NS_PAGE_RADIO;
            } else a->page = a->menu == 4 ? NS_PAGE_SETTINGS : (a->menu == 5 ? NS_PAGE_HISTORY : (a->menu == 6 ? NS_PAGE_DIAGNOSTICS : (a->menu == 7 ? NS_PAGE_FILTERS : NS_PAGE_EXPORTS)));
        }
    } else if(a->page == NS_PAGE_SELECTOR && key == NS_KEY_OK) {
        return ns_app_start(a, a->selected, a->ble, true, now);
    } else if(a->page == NS_PAGE_RADIO && key == NS_KEY_OK) {
        if(a->capabilities.esp_info_only && a->selected == NS_RADIO_ESP &&
           (a->info_pending || a->session.active != NS_RADIO_NONE)) return NS_BUSY;
        if(a->session.active != NS_RADIO_NONE) return ns_app_stop(a, now);
        if(a->selected == NS_RADIO_ESP) {
            if(a->capabilities.esp_info_only && !a->ble) {
                if(a->info_pending) return a->status = NS_BUSY;
                a->info_pending = 2; return a->status = NS_OK;
            }
            return ns_app_start(a, NS_RADIO_ESP, a->ble, false, now);
        }
        if(!(a->selected == NS_RADIO_NRF ? a->capabilities.nrf : a->capabilities.cc)) return a->status = NS_UNSUPPORTED;
        a->page = NS_PAGE_SELECTOR; /* confirmation is consumed by one start only */
    } else if(a->page == NS_PAGE_RADIO && a->selected == NS_RADIO_ESP && !a->ble) {
        if(key == NS_KEY_RIGHT) {
            if(a->session.ap_count) a->wifi_details = !a->wifi_details;
            else {
                if(a->capabilities.esp_info_only) {
                    if(a->info_pending || a->session.active != NS_RADIO_NONE) return a->status = NS_BUSY;
                    a->info_pending = 2; return a->status = NS_OK;
                }
                return ns_app_device_info(a, now);
            }
        }
        if(a->capabilities.esp_info_only && (key == NS_KEY_UP || key == NS_KEY_DOWN))
            a->info_view = (a->info_view + (key == NS_KEY_DOWN ? 1 : 2)) % 3;
        if(key == NS_KEY_LEFT) ns_app_baseline(a);
        if(key == NS_KEY_UP && a->ap_index) a->ap_index--;
        if(key == NS_KEY_DOWN && !a->capabilities.esp_info_only) {
            uint8_t indices[NS_AP_LIMIT]; size_t count = 0;
            if(ns_observation_select(a->session.aps, a->session.ap_seen_ms, a->session.ap_count,
               &a->observation_filter, now, indices, sizeof(indices), &count) == NS_OK && a->ap_index+1 < count) a->ap_index++;
        }
    } else if(a->page == NS_PAGE_RADIO && a->selected == NS_RADIO_NRF) {
        if(key == NS_KEY_RIGHT) a->nrf_details = !a->nrf_details;
    } else if(a->page == NS_PAGE_RADIO && a->selected == NS_RADIO_CC) {
        uint32_t frequency = a->session.active == NS_RADIO_CC ? a->session.cc.frequency : a->saved.settings.cc_frequency_hz;
        if(key == NS_KEY_LEFT || key == NS_KEY_RIGHT) return ns_app_cc_tune(a,
            key == NS_KEY_RIGHT ? frequency+100000 : frequency-100000, now);
        if(key == NS_KEY_UP) {
            static const uint32_t presets[] = {315000000, 433920000, 868000000, 915000000};
            unsigned i = 0; while(i < 4 && presets[i] <= frequency) i++;
            return ns_app_cc_tune(a, presets[i % 4], now);
        }
        if(key == NS_KEY_DOWN) {
            a->cc_details = !a->cc_details;
            if(a->cc_details && a->session.active == NS_RADIO_CC) {
                NsResult r = ns_session_cc_diagnose(&a->session, now);
                if(a->session.active == NS_RADIO_NONE) {
                    a->history_dirty = true; NsResult stored = persist_history(a); if(stored != NS_OK) r = stored;
                }
                return a->status = r;
            }
        }
    } else if(a->page == NS_PAGE_SETTINGS) {
        if(key == NS_KEY_UP && a->setting) a->setting--;
        if(key == NS_KEY_DOWN && a->setting < 2) a->setting++;
        if(key == NS_KEY_LEFT || key == NS_KEY_RIGHT) return change_setting(a, key == NS_KEY_RIGHT);
    } else if(a->page == NS_PAGE_HISTORY) {
        if(key == NS_KEY_UP && a->history_index) a->history_index--;
        if(key == NS_KEY_DOWN && ns_history_select(&a->saved, &a->history_filter, a->history_index+1)) a->history_index++;
        if(key == NS_KEY_RIGHT || key == NS_KEY_LEFT) {
            a->history_filter.radio = (NsRadio)(((unsigned)a->history_filter.radio + (key == NS_KEY_RIGHT ? 1 : 3)) % 4);
            a->history_index = 0;
        }
        if(key == NS_KEY_OK) { a->history_filter.failures_only = !a->history_filter.failures_only; a->history_index = 0; }
    } else if(a->page == NS_PAGE_FILTERS) {
        if(key == NS_KEY_UP && a->filter_setting) a->filter_setting--;
        if(key == NS_KEY_DOWN && a->filter_setting < 2) a->filter_setting++;
        if(key == NS_KEY_OK) { a->observation_filter = ns_observation_filter_default(); a->ap_index = 0; }
        if(key == NS_KEY_RIGHT || key == NS_KEY_LEFT) {
            bool up = key == NS_KEY_RIGHT;
            if(a->filter_setting == 0) a->observation_filter.channel = (uint8_t)((a->observation_filter.channel+(up ? 1 : 14))%15);
            else if(a->filter_setting == 1) {
                int value = a->observation_filter.minimum_rssi + (up ? 5 : -5);
                a->observation_filter.minimum_rssi = (int16_t)(value < -127 ? -127 : (value > 20 ? 20 : value));
            } else {
                uint32_t age = a->observation_filter.maximum_age_ms;
                a->observation_filter.maximum_age_ms = up ? (age < 600000 ? age+10000 : 600000) : (age > 10000 ? age-10000 : 0);
            }
            a->ap_index = 0;
        }
    } else if(a->page == NS_PAGE_EXPORTS && key == NS_KEY_OK) return ns_app_export_history(a, now);
    else if(a->page == NS_PAGE_DIAGNOSTICS && key == NS_KEY_OK) return a->history_dirty ? (a->status = persist_history(a)) : ns_app_reload(a);
    return NS_OK;
}
void ns_app_frame(const NsApplication* a, NsFrame* f) {
    memset(f, 0, sizeof(*f));
    snprintf(f->rows[0], sizeof(f->rows[0]), "NetSentinel Ultra");
    snprintf(f->rows[5], sizeof(f->rows[5]), "%s", ns_result_name(a->status));
    if(a->page == NS_PAGE_HOME) {
        unsigned first = a->menu < 4 ? 0 : a->menu-3;
        for(unsigned i = 0; i < 4; i++) snprintf(f->rows[i+1], sizeof(f->rows[i+1]), "%c %s", a->menu == first+i ? '>' : ' ', a->capabilities.esp_info_only && first+i == 0 ? "ESP32 diagnostics" : menu[first+i]);
    } else if(a->page == NS_PAGE_SELECTOR) {
        snprintf(f->rows[1], sizeof(f->rows[1]), "Set selector to %s", a->selected == NS_RADIO_NRF ? "NRF24" : "CC1101");
        snprintf(f->rows[2], sizeof(f->rows[2]), "Stop before changing it");
        snprintf(f->rows[3], sizeof(f->rows[3]), "OK confirms and starts RX");
        snprintf(f->rows[4], sizeof(f->rows[4]), "Back cancels");
    } else if(a->page == NS_PAGE_RADIO) {
        if(a->status == NS_OK) snprintf(f->rows[5], sizeof(f->rows[5]), "%s", a->session.active == NS_RADIO_NONE ? "Idle: OK starts" : "RX: OK stops");
        if(a->selected == NS_RADIO_ESP) {
            if(a->ble) snprintf(f->rows[1], sizeof(f->rows[1]), "BLE active scan: disabled");
            else if(a->session.ap_count) {
                uint8_t indices[NS_AP_LIMIT]; size_t count = 0;
                NsResult filtered = ns_observation_select(a->session.aps, a->session.ap_seen_ms,
                    a->session.ap_count, &a->observation_filter, a->now_ms, indices, sizeof(indices), &count);
                if(filtered != NS_OK || !count) {
                    snprintf(f->rows[1], sizeof(f->rows[1]), "No observations match filters");
                    snprintf(f->rows[4], sizeof(f->rows[4]), "Filters: channel/RSSI/age");
                } else {
                    size_t index = indices[a->ap_index < count ? a->ap_index : 0];
                    const NsAccessPoint* ap = &a->session.aps[index];
                    uint8_t counts[14]; ns_channel_counts(a->session.aps, a->session.ap_count, counts);
                    NsDiff d = ns_diff(a->baseline, a->baseline_count, a->session.aps, a->session.ap_count);
                    snprintf(f->rows[1], sizeof(f->rows[1]), "AP %u (identifiers hidden)", (unsigned)index+1);
                    snprintf(f->rows[2], sizeof(f->rows[2]), "Ch%u %d dBm APs on ch:%u", ap->channel, ap->rssi, counts[ap->channel-1]);
                    snprintf(f->rows[3], sizeof(f->rows[3]), "New:%u Unseen:%u Moved:%u", d.new_aps, d.missing_aps, d.channel_changes);
                    snprintf(f->rows[4], sizeof(f->rows[4]), "RSSI min:%d max:%d", a->session.ap_min[index], a->session.ap_max[index]);
                    if(a->wifi_details) {
                        snprintf(f->rows[3], sizeof(f->rows[3]), "BSSID: hidden (RAM matching)");
                        snprintf(f->rows[4], sizeof(f->rows[4]), "Security: %s", ap->security[0] ? "reported" : "not reported");
                    }
                }
            } else {
                snprintf(f->rows[1], sizeof(f->rows[1]), "Wi-Fi %s", a->capabilities.esp_wifi_verified ? "passive adapter" : "command capture needed");
                snprintf(f->rows[2], sizeof(f->rows[2]), "%.23s", a->session.marauder.version);
                snprintf(f->rows[3], sizeof(f->rows[3]), "%.38s", a->session.marauder.hardware);
                snprintf(f->rows[4], sizeof(f->rows[4]), "%s", a->capabilities.esp_uart ?
                    "Right: info  OK: discovery" : "UART disabled in this build");
                const char* state = NULL;
                if(!a->capabilities.esp_uart) {
                    if(a->status == NS_OK || a->status == NS_ABSENT || a->status == NS_UNSUPPORTED) state = "ESP32 adapter disabled";
                }
                else if(a->session.active == NS_RADIO_ESP && a->session.marauder.stage == NS_M_INFO) state = "Waiting for info reply";
                else if(a->status == NS_TIMEOUT) state = "No reply: check UART";
                else if(a->status == NS_FULL) state = "UART buffer/log full";
                else if(a->status == NS_INVALID) state = "Reply/state invalid";
                else if(a->status == NS_IO) state = "UART or SD I/O error";
                else if(a->status == NS_BUSY) state = "Port/session busy";
                else if(a->status == NS_UNSUPPORTED) state = "Scan not yet verified";
                else if(a->session.marauder.stage == NS_M_DONE && a->status == NS_OK) state = a->session.marauder.info_complete ? "Info reply received" : "Incomplete info status";
                else if(a->status == NS_OK) state = "Right queries device info";
                if(a->capabilities.esp_info_only) {
                    snprintf(f->rows[1], sizeof(f->rows[1]), "ESP32 info-only USART");
                    /* Only known matched constants reach diagnostics, never untrusted identity text. */
                    snprintf(f->rows[2], sizeof(f->rows[2]), "%s", a->info_validated ? "Marauder v1.11.0 (last success)" : "Identity not validated");
                    snprintf(f->rows[3], sizeof(f->rows[3]), "%s", a->info_validated ? "Marauder v4 (last success)" : "");
                    if(a->info_view == 1) {
                        snprintf(f->rows[1], sizeof(f->rows[1]), "ESP-IDF (validated reply)");
                        snprintf(f->rows[2], sizeof(f->rows[2]), "%s", a->info_validated ? "v5.5.1-710-g8410210c9a" : "Not validated");
                        snprintf(f->rows[3], sizeof(f->rows[3]), "%s", a->info_validated ? (a->info_sd_connected ? "SD: Connected (last success)" : "SD: Disconnected (last success)") : "SD: unknown");
                    } else if(a->info_view == 2) {
                        snprintf(f->rows[1], sizeof(f->rows[1]), "Last success: %lu (session)", (unsigned long)a->info_success_id);
                        snprintf(f->rows[2], sizeof(f->rows[2]), "Elapsed: %lu ms%s", (unsigned long)a->info_elapsed_ms, a->session.active == NS_RADIO_ESP ? " (running)" : "");
                        snprintf(f->rows[3], sizeof(f->rows[3]), "Timeout: %lu ms", (unsigned long)a->session.timeout_ms);
                    }
                    snprintf(f->rows[4], sizeof(f->rows[4]), "%s", a->info_pending || a->session.active != NS_RADIO_NONE ? "Retry disabled; Back: cancel" : "OK: Retry  Up/Down: details");
                    if(a->info_pending) state = "Connecting: USART requested";
                    else if(a->session.active == NS_RADIO_ESP) state = "Waiting: info response";
                    else if(a->status == NS_TIMEOUT) state = "Timeout: link unconfirmed";
                    else if(a->status == NS_BUSY) state = "Error: USART/session busy";
                    else if(a->status == NS_FULL) state = "Error: response/storage full";
                    else if(a->status == NS_INVALID) state = "Error: reply/state invalid";
                    else if(a->status == NS_IO) state = "Error: serial or SD I/O";
                    else if(a->status == NS_UNSUPPORTED) state = "Error: USART configuration";
                    else if(a->status != NS_OK && a->status != NS_ABSENT) state = "Error: operation failed";
                    else if(a->info_cancelled) state = "Cancelled: USART closed";
                    else if(a->session.marauder.stage == NS_M_DONE && a->session.marauder.info_complete)
                        state = a->session.marauder.identity_matches ? "Connected: query passed" : "Error: identity mismatch";
                    else if(a->session.marauder.stage == NS_M_DONE && a->session.marauder.info_fields) state = "Error: incomplete reply";
                    else state = "Disconnected: USART closed";
                }
                if(state) snprintf(f->rows[5], sizeof(f->rows[5]), "%s", state);
            }
        } else if(a->selected == NS_RADIO_NRF) {
            snprintf(f->rows[1], sizeof(f->rows[1]), "NRF24 sampled activity");
            f->bar_count = NS_NRF_CHANNELS;
            for(unsigned ch = 0; ch < NS_NRF_CHANNELS; ch++) f->bars[ch] = ns_graph_height(a->session.nrf.hits[ch], a->session.nrf.samples[ch], 22);
            NsActivitySummary summary;
            if(a->nrf_details && ns_activity_summary(&a->session.nrf, &summary) == NS_OK) {
                f->bar_count = 0;
                snprintf(f->rows[2], sizeof(f->rows[2]), "Peak ch%u: %u%%", summary.peak_channel, summary.peak_percent);
                snprintf(f->rows[3], sizeof(f->rows[3]), "Quiet ch%u: %u%%", summary.quiet_channel, summary.quiet_percent);
                snprintf(f->rows[4], sizeof(f->rows[4]), "Measured %u/126; Right: plot", summary.measured_channels);
            } else snprintf(f->rows[4], sizeof(f->rows[4]), "0-125 %lu samples; Right: stats", (unsigned long)a->session.observations);
            if(!a->capabilities.nrf) {
                f->bar_count = 0;
                snprintf(f->rows[2], sizeof(f->rows[2]), "SPI/CE routing unverified");
                snprintf(f->rows[4], sizeof(f->rows[4]), "Physical adapter blocked");
            }
        } else {
            const NsCc* c = &a->session.cc;
            uint32_t frequency = c->frequency ? c->frequency : a->saved.settings.cc_frequency_hz;
            snprintf(f->rows[1], sizeof(f->rows[1]), "CC1101 %lu Hz", (unsigned long)frequency);
            if(a->cc_details) {
                if(c->diagnostic_valid) {
                    snprintf(f->rows[2], sizeof(f->rows[2]), "ID %02X/%02X State %02X", c->diagnostic.part, c->diagnostic.version, c->diagnostic.state);
                    snprintf(f->rows[3], sizeof(f->rows[3]), "Reads%lu RSSIraw %02X", (unsigned long)c->diagnostic.transactions, c->diagnostic.rssi_raw);
                } else snprintf(f->rows[2], sizeof(f->rows[2]), "Raw SPI diagnostic unavailable");
                snprintf(f->rows[4], sizeof(f->rows[4]), "Down: trace Up: preset");
            } else if(c->samples) {
                snprintf(f->rows[2], sizeof(f->rows[2]), "RSSI %d Mean %d dBm", c->last, ns_cc_recent_mean(c));
                f->cc_plot = true; f->bar_count = c->trace_count;
                size_t first = (c->trace_next + NS_CC_TRACE-c->trace_count) % NS_CC_TRACE;
                for(size_t i = 0; i < c->trace_count; i++) f->bars[i] = ns_cc_graph_height(c->trace[(first+i) % NS_CC_TRACE], 10);
                snprintf(f->rows[4], sizeof(f->rows[4]), "Lo%d Hi%d >=%d:%u%%", c->minimum, c->maximum, NS_CC_THRESHOLD_DBM, ns_cc_above_percent(c, NS_CC_THRESHOLD_DBM));
            } else {
                snprintf(f->rows[2], sizeof(f->rows[2]), "%s", a->capabilities.cc ? "No measurements yet" : "External RX adapter pending");
                snprintf(f->rows[3], sizeof(f->rows[3]), "Left/Right: 100 kHz");
                snprintf(f->rows[4], sizeof(f->rows[4]), "Up: preset Down: diagnostics");
            }
        }
    } else if(a->page == NS_PAGE_SETTINGS) {
        snprintf(f->rows[1], sizeof(f->rows[1]), "%c Timeout %lu ms", a->setting == 0 ? '>' : ' ', (unsigned long)a->saved.settings.timeout_ms);
        snprintf(f->rows[2], sizeof(f->rows[2]), "%c NRF dwell %lu us", a->setting == 1 ? '>' : ' ', (unsigned long)a->saved.settings.dwell_us);
        snprintf(f->rows[3], sizeof(f->rows[3]), "%c CC %lu Hz", a->setting == 2 ? '>' : ' ', (unsigned long)a->saved.settings.cc_frequency_hz);
        snprintf(f->rows[4], sizeof(f->rows[4]), "Left/Right: change + save");
    } else if(a->page == NS_PAGE_HISTORY) {
        const NsHistory* h = ns_history_select(&a->saved, &a->history_filter, a->history_index);
        snprintf(f->rows[0], sizeof(f->rows[0]), "History R%u %s", (unsigned)a->history_filter.radio, a->history_filter.failures_only ? "errors" : "all");
        if(h) {
            snprintf(f->rows[1], sizeof(f->rows[1]), "Session %lu radio %u", (unsigned long)h->id, (unsigned)h->radio);
            snprintf(f->rows[2], sizeof(f->rows[2]), "%lu observations", (unsigned long)h->observations);
            snprintf(f->rows[3], sizeof(f->rows[3]), "Result: %s", ns_result_name(h->result));
            snprintf(f->rows[4], sizeof(f->rows[4]), "Duration %lu ms", (unsigned long)(h->ended_ms-h->started_ms));
        } else snprintf(f->rows[1], sizeof(f->rows[1]), "No completed sessions");
    } else if(a->page == NS_PAGE_EXPORTS) {
        snprintf(f->rows[1], sizeof(f->rows[1]), "Filtered numeric history CSV");
        snprintf(f->rows[2], sizeof(f->rows[2]), "OK: export; no radio access");
        snprintf(f->rows[3], sizeof(f->rows[3]), "Last export ID: %lu", (unsigned long)a->export_id);
        snprintf(f->rows[4], sizeof(f->rows[4]), "SD apps_data/netsentinel");
    } else if(a->page == NS_PAGE_FILTERS) {
        snprintf(f->rows[1], sizeof(f->rows[1]), "%c Channel %u (0=all)", a->filter_setting == 0 ? '>' : ' ', a->observation_filter.channel);
        snprintf(f->rows[2], sizeof(f->rows[2]), "%c Min RSSI %d dBm", a->filter_setting == 1 ? '>' : ' ', a->observation_filter.minimum_rssi);
        snprintf(f->rows[3], sizeof(f->rows[3]), "%c Age %lu ms (0=all)", a->filter_setting == 2 ? '>' : ' ', (unsigned long)a->observation_filter.maximum_age_ms);
        snprintf(f->rows[4], sizeof(f->rows[4]), "L/R change; OK clears (RAM)");
    } else {
        snprintf(f->rows[1], sizeof(f->rows[1]), "NRF:%s CC:%s", a->capabilities.nrf ? "port ready" : "no port", a->capabilities.cc ? "port ready" : "no port");
        snprintf(f->rows[2], sizeof(f->rows[2]), "CSV: %s", a->session.logging ? "open" : "closed");
        snprintf(f->rows[3], sizeof(f->rows[3]), "History: %s", a->history_dirty ? "unsaved; OK retries" : "saved");
        snprintf(f->rows[4], sizeof(f->rows[4]), "Gen %lu  OK: retry SD", (unsigned long)a->persistence.generation);
    }
}
