#include "../../include/netsentinel/application.h"
#include <stdio.h>
#include <string.h>
static const char* const menu[] = {"Wi-Fi", "BLE", "NRF24 activity", "CC1101 RSSI", "Settings", "History", "Diagnostics"};
const char* ns_result_name(NsResult r) {
    static const char* const names[] = {"OK", "Invalid", "Busy", "Unsupported", "I/O failure", "Absent", "Timeout", "Full"};
    return r >= NS_OK && r <= NS_FULL ? names[r] : "Unknown";
}
static void apply_settings(NsApplication* a) {
    a->session.timeout_ms = a->saved.settings.timeout_ms;
    a->session.dwell_us = a->saved.settings.dwell_us;
}
NsResult ns_app_init(NsApplication* a, const NsPlatform* io, const NsStore* store, NsCapabilities caps) {
    if(!a || !io || !store) return NS_INVALID;
    memset(a, 0, sizeof(*a)); a->capabilities = caps; a->running = true;
    ns_saved_defaults(&a->saved);
    NsResult r = ns_persistence_load(&a->persistence, store, &a->saved);
    ns_session_init(&a->session, io, NULL); /* ESP deliberately disabled */
    a->session.id = a->saved.last_id;
    a->session.history_count = a->saved.history_count;
    a->session.history_next = a->saved.history_count % NS_HISTORY_LIMIT;
    memcpy(a->session.history, a->saved.history, sizeof(a->saved.history));
    apply_settings(a); a->status = r == NS_ABSENT ? NS_OK : r;
    return r;
}
NsResult ns_app_reload(NsApplication* a) {
    if(!a) return NS_INVALID;
    if(a->session.active != NS_RADIO_NONE || a->history_dirty) return a->status = NS_BUSY;
    NsSavedState next; NsPersistence loaded;
    NsResult r = ns_persistence_load(&loaded, a->persistence.io, &next);
    if(r == NS_OK || r == NS_ABSENT) {
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
    if(a->session.active != NS_RADIO_NONE) return a->status = NS_BUSY;
    NsResult r = persist_history(a);
    if(r != NS_OK) return a->status = r;
    NsSavedState next = a->saved; next.settings = *settings;
    r = ns_persistence_save(&a->persistence, &next);
    if(r == NS_OK) { a->saved = next; apply_settings(a); }
    return a->status = r;
}
NsResult ns_app_start(NsApplication* a, NsRadio radio, bool ble, bool confirmed, uint32_t now) {
    if(!a || !a->running) return NS_INVALID;
    if(a->session.active != NS_RADIO_NONE) return a->status = NS_BUSY;
    if(radio == NS_RADIO_ESP) return a->status = NS_UNSUPPORTED;
    if(radio != NS_RADIO_NRF && radio != NS_RADIO_CC) return a->status = NS_INVALID;
    if(!(radio == NS_RADIO_NRF ? a->capabilities.nrf : a->capabilities.cc)) return a->status = NS_UNSUPPORTED;
    if(!confirmed) return a->status = NS_INVALID;
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
NsResult ns_app_stop(NsApplication* a, uint32_t now) {
    if(!a) return NS_INVALID;
    if(a->session.active == NS_RADIO_NONE && !a->history_dirty) return a->status;
    NsResult session_result = NS_OK;
    if(a->session.active != NS_RADIO_NONE) {
        ns_session_stop(&a->session, now); a->history_dirty = true;
        session_result = a->session.error;
    }
    NsResult r = persist_history(a);
    return a->status = r != NS_OK ? r : session_result;
}
NsResult ns_app_step(NsApplication* a, uint32_t now) {
    if(!a || !a->running) return NS_INVALID;
    if(a->session.active == NS_RADIO_NONE) return NS_OK;
    NsResult r = ns_session_tick(&a->session, now);
    if(a->session.active == NS_RADIO_NONE) {
        a->history_dirty = true;
        NsResult stored = persist_history(a);
        if(stored != NS_OK) r = stored;
    }
    return a->status = r;
}
NsResult ns_app_feed(NsApplication* a, const uint8_t* bytes, size_t n, uint32_t now) {
    if(!a) return NS_INVALID;
    bool active = a->session.active != NS_RADIO_NONE;
    NsResult r = ns_session_feed(&a->session, bytes, n, now);
    if(active && a->session.active == NS_RADIO_NONE) {
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
    if(!a || !a->running || key > NS_KEY_BACK || key < NS_KEY_UP) return NS_INVALID;
    if(key == NS_KEY_BACK) {
        NsResult r = ns_app_stop(a, now);
        if(a->page == NS_PAGE_HOME) a->running = false;
        else a->page = NS_PAGE_HOME;
        return r;
    }
    if(a->page == NS_PAGE_HOME) {
        if(key == NS_KEY_UP && a->menu) a->menu--;
        if(key == NS_KEY_DOWN && a->menu < 6) a->menu++;
        if(key == NS_KEY_OK) {
            a->status = NS_OK;
            if(a->menu < 4) {
                a->selected = a->menu < 2 ? NS_RADIO_ESP : (a->menu == 2 ? NS_RADIO_NRF : NS_RADIO_CC);
                a->ble = a->menu == 1; a->page = NS_PAGE_RADIO;
            } else a->page = a->menu == 4 ? NS_PAGE_SETTINGS : (a->menu == 5 ? NS_PAGE_HISTORY : NS_PAGE_DIAGNOSTICS);
        }
    } else if(a->page == NS_PAGE_SELECTOR && key == NS_KEY_OK) {
        return ns_app_start(a, a->selected, a->ble, true, now);
    } else if(a->page == NS_PAGE_RADIO && key == NS_KEY_OK) {
        if(a->session.active != NS_RADIO_NONE) return ns_app_stop(a, now);
        if(a->selected == NS_RADIO_ESP || !(a->selected == NS_RADIO_NRF ? a->capabilities.nrf : a->capabilities.cc)) return a->status = NS_UNSUPPORTED;
        a->page = NS_PAGE_SELECTOR; /* confirmation is consumed by one start only */
    } else if(a->page == NS_PAGE_SETTINGS) {
        if(key == NS_KEY_UP && a->setting) a->setting--;
        if(key == NS_KEY_DOWN && a->setting < 2) a->setting++;
        if(key == NS_KEY_LEFT || key == NS_KEY_RIGHT) return change_setting(a, key == NS_KEY_RIGHT);
    } else if(a->page == NS_PAGE_HISTORY) {
        if(key == NS_KEY_UP && a->history_index) a->history_index--;
        if(key == NS_KEY_DOWN && a->history_index+1 < a->saved.history_count) a->history_index++;
    } else if(a->page == NS_PAGE_DIAGNOSTICS && key == NS_KEY_OK) return a->history_dirty ? (a->status = persist_history(a)) : ns_app_reload(a);
    return NS_OK;
}
void ns_app_frame(const NsApplication* a, NsFrame* f) {
    memset(f, 0, sizeof(*f));
    snprintf(f->rows[0], sizeof(f->rows[0]), "NetSentinel Ultra");
    snprintf(f->rows[5], sizeof(f->rows[5]), "%s", ns_result_name(a->status));
    if(a->page == NS_PAGE_HOME) {
        unsigned first = a->menu < 4 ? 0 : 3;
        for(unsigned i = 0; i < 4; i++) snprintf(f->rows[i+1], sizeof(f->rows[i+1]), "%c %s", a->menu == first+i ? '>' : ' ', menu[first+i]);
    } else if(a->page == NS_PAGE_SELECTOR) {
        snprintf(f->rows[1], sizeof(f->rows[1]), "Set selector to %s", a->selected == NS_RADIO_NRF ? "NRF24" : "CC1101");
        snprintf(f->rows[2], sizeof(f->rows[2]), "Stop before changing it");
        snprintf(f->rows[3], sizeof(f->rows[3]), "OK confirms and starts RX");
        snprintf(f->rows[4], sizeof(f->rows[4]), "Back cancels");
    } else if(a->page == NS_PAGE_RADIO) {
        if(a->status == NS_OK) snprintf(f->rows[5], sizeof(f->rows[5]), "%s", a->session.active == NS_RADIO_NONE ? "Idle: OK starts" : "RX: OK stops");
        if(a->selected == NS_RADIO_ESP) {
            snprintf(f->rows[1], sizeof(f->rows[1]), "%s scanning disabled", a->ble ? "BLE" : "Wi-Fi");
            snprintf(f->rows[2], sizeof(f->rows[2]), "Passive firmware unverified");
            snprintf(f->rows[3], sizeof(f->rows[3]), "Back: dashboard");
        } else if(a->selected == NS_RADIO_NRF) {
            snprintf(f->rows[1], sizeof(f->rows[1]), "NRF24 sampled activity");
            f->bar_count = NS_NRF_CHANNELS;
            for(unsigned ch = 0; ch < NS_NRF_CHANNELS; ch++) f->bars[ch] = ns_graph_height(a->session.nrf.hits[ch], a->session.nrf.samples[ch], 22);
            snprintf(f->rows[4], sizeof(f->rows[4]), "0-125  %lu observations", (unsigned long)a->session.observations);
        } else {
            snprintf(f->rows[1], sizeof(f->rows[1]), "CC1101 %lu Hz", (unsigned long)a->saved.settings.cc_frequency_hz);
            if(a->session.cc.samples) {
                snprintf(f->rows[2], sizeof(f->rows[2]), "RSSI %d dBm", a->session.cc.last);
                snprintf(f->rows[3], sizeof(f->rows[3]), "Min %d Max %d", a->session.cc.minimum, a->session.cc.maximum);
            } else snprintf(f->rows[2], sizeof(f->rows[2]), "No measurements yet");
            snprintf(f->rows[4], sizeof(f->rows[4]), "OK: start/stop RX");
        }
    } else if(a->page == NS_PAGE_SETTINGS) {
        snprintf(f->rows[1], sizeof(f->rows[1]), "%c Timeout %lu ms", a->setting == 0 ? '>' : ' ', (unsigned long)a->saved.settings.timeout_ms);
        snprintf(f->rows[2], sizeof(f->rows[2]), "%c NRF dwell %lu us", a->setting == 1 ? '>' : ' ', (unsigned long)a->saved.settings.dwell_us);
        snprintf(f->rows[3], sizeof(f->rows[3]), "%c CC %lu Hz", a->setting == 2 ? '>' : ' ', (unsigned long)a->saved.settings.cc_frequency_hz);
        snprintf(f->rows[4], sizeof(f->rows[4]), "Left/Right: change + save");
    } else if(a->page == NS_PAGE_HISTORY) {
        if(a->history_index < a->saved.history_count) {
            const NsHistory* h = &a->saved.history[a->saved.history_count-1-a->history_index];
            snprintf(f->rows[1], sizeof(f->rows[1]), "Session %lu radio %u", (unsigned long)h->id, (unsigned)h->radio);
            snprintf(f->rows[2], sizeof(f->rows[2]), "%lu observations", (unsigned long)h->observations);
            snprintf(f->rows[3], sizeof(f->rows[3]), "Result: %s", ns_result_name(h->result));
            snprintf(f->rows[4], sizeof(f->rows[4]), "Duration %lu ms", (unsigned long)(h->ended_ms-h->started_ms));
        } else snprintf(f->rows[1], sizeof(f->rows[1]), "No completed sessions");
    } else {
        snprintf(f->rows[1], sizeof(f->rows[1]), "NRF:%s CC:%s", a->capabilities.nrf ? "port ready" : "no port", a->capabilities.cc ? "port ready" : "no port");
        snprintf(f->rows[2], sizeof(f->rows[2]), "CSV: %s", a->session.logging ? "open" : "closed");
        snprintf(f->rows[3], sizeof(f->rows[3]), "History: %s", a->history_dirty ? "unsaved; OK retries" : "saved");
        snprintf(f->rows[4], sizeof(f->rows[4]), "Gen %lu  OK: retry SD", (unsigned long)a->persistence.generation);
    }
}
