#include "ap_parser.h"
#include "ns_analysis.h"

#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_power.h>
#include <furi_hal_serial.h>
#include <furi_hal_serial_control.h>
#include <expansion/expansion.h>
#include <gui/gui.h>
#include <input/input.h>
#include <storage/storage.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* NetSentinel Ultra v0.2. No packet injection or arbitrary CLI execution.
   Built for Sacred Labs Ultra V3 ESP32 Marauder over 115200 UART. */
#define NS_MAX_APS 48
#define NS_SERIAL_BAUD 115200
#define NS_RX_CAPACITY 4096
#define NS_LINE_SIZE 256
#define NS_RAW_ROWS 6
#define NS_RAW_WIDTH 30
#define NS_MENU_ITEMS 9
#define NS_EXPORT_DIR "/ext/apps_data/netsentinel"

typedef enum {
    PageMenu,
    PageWifi,
    PageWifiDetail,
    PageChannel,
    PageClients,
    PageBluetooth,
    PageBaseline,
    PageExport,
    PageInfo,
    PageRaw,
    PageHardware,
} NsPage;

typedef enum { ScanNone, ScanWifi, ScanClients, ScanBluetooth } NsScan;

typedef struct {
    FuriMutex* mutex;
    FuriMessageQueue* inputs;
    FuriStreamBuffer* rx;
    ViewPort* port;
    Gui* gui;
    FuriHalSerialHandle* serial;
    NsAccessPoint aps[NS_MAX_APS];
    NsAccessPoint baseline[NS_MAX_APS];
    uint8_t ap_count;
    uint8_t baseline_count;
    bool has_baseline;
    uint8_t wifi_index;
    uint8_t menu_index;
    uint8_t channel_index; /* 0..13 */
    uint8_t hardware_index; /* 0..2 */
    NsPage page;
    NsScan scanning;
    NsScan listing_pending;
    uint32_t list_at_tick;
    uint32_t last_request_tick;
    bool requested;
    bool rx_since_request;
    bool ever_rx;
    volatile uint32_t dropped_bytes;
    uint32_t serial_lines;
    uint16_t overflow_lines;
    char line[NS_LINE_SIZE];
    uint16_t line_len;
    bool line_overflow;
    char raw[NS_RAW_ROWS][NS_RAW_WIDTH + 1];
    uint8_t raw_next;
    char notice[40];
    char export_name[24];
    bool export_ok;
} NsApp;

static const char* const ns_menu[NS_MENU_ITEMS] = {
    "Wi-Fi Explorer", "Channel Analyzer", "Client Observations", "Bluetooth Monitor",
    "Network Baseline", "Export Wi-Fi CSV", "ESP32 Information", "UART Diagnostics", "Ultra Hardware"
};

static void ns_uart_send(NsApp* app, const char* command) {
    if(!app->serial) return;
    furi_hal_serial_tx(app->serial, (const uint8_t*)command, strlen(command));
    furi_hal_serial_tx_wait_complete(app->serial);
    app->last_request_tick = furi_get_tick();
    app->requested = true;
    app->rx_since_request = false;
}

static void ns_note(NsApp* app, const char* message) {
    snprintf(app->notice, sizeof(app->notice), "%s", message);
}

static void ns_begin(NsApp* app, NsScan scan) {
    if(!app->serial || app->scanning != ScanNone) return;
    app->listing_pending = ScanNone;
    app->serial_lines = 0;
    app->raw_next = 0;
    memset(app->raw, 0, sizeof(app->raw));
    if(scan == ScanWifi) {
        app->ap_count = 0;
        app->wifi_index = 0;
        memset(app->aps, 0, sizeof(app->aps));
    }
    app->scanning = scan;
    ns_note(app, "Active scan");
    if(scan == ScanWifi) ns_uart_send(app, "scanap\n");
    else if(scan == ScanClients) ns_uart_send(app, "scansta\n");
    else if(scan == ScanBluetooth) ns_uart_send(app, "sniffbt\n");
}

static void ns_stop(NsApp* app) {
    if(app->scanning == ScanNone) return;
    NsScan former = app->scanning;
    app->scanning = ScanNone;
    ns_uart_send(app, "stopscan\n");
    /* The list is requested only after Marauder has had time to stop. */
    app->listing_pending = former == ScanBluetooth ? ScanNone : former;
    app->list_at_tick = furi_get_tick() + furi_ms_to_ticks(600);
    ns_note(app, "Scan stopped");
}

static void ns_toggle(NsApp* app, NsScan wanted) {
    if(app->scanning == wanted) ns_stop(app);
    else if(app->scanning == ScanNone) ns_begin(app, wanted);
    else ns_note(app, "Stop active scan first");
}

static void ns_add_ap(NsApp* app, const NsAccessPoint* new_ap) {
    for(uint8_t i = 0; i < app->ap_count; i++) {
        NsAccessPoint* old = &app->aps[i];
        bool mac_match = new_ap->bssid[0] && old->bssid[0] &&
                         strcmp(new_ap->bssid, old->bssid) == 0;
        bool weak_match = (!new_ap->bssid[0] || !old->bssid[0]) &&
                          new_ap->channel == old->channel &&
                          strcmp(new_ap->ssid, old->ssid) == 0;
        if(mac_match || weak_match) {
            if(new_ap->bssid[0]) memcpy(old->bssid, new_ap->bssid, NS_BSSID_SIZE);
            old->rssi = new_ap->rssi;
            old->channel = new_ap->channel;
            return;
        }
    }
    if(app->ap_count < NS_MAX_APS) app->aps[app->ap_count++] = *new_ap;
}

/* Called by main thread only, with mutex held. */
static void ns_finish_line(NsApp* app) {
    app->line[app->line_len] = '\0';
    if(app->line_len && !app->line_overflow) {
        char* target = app->raw[app->raw_next];
        size_t x = 0;
        for(size_t j = 0; app->line[j] && x < NS_RAW_WIDTH; ++j) {
            unsigned char c = (unsigned char)app->line[j];
            target[x++] = c >= 32 && c <= 126 ? (char)c : '?';
        }
        target[x] = '\0';
        app->raw_next = (app->raw_next + 1) % NS_RAW_ROWS;
        app->serial_lines++;
        /* Only AP-related sessions populate Wi-Fi AP list. */
        if(app->scanning == ScanWifi || app->listing_pending == ScanWifi ||
           (app->page == PageWifi && app->scanning == ScanNone)) {
            NsAccessPoint found;
            if(ns_parse_ap_line(app->line, &found)) ns_add_ap(app, &found);
        }
    } else if(app->line_overflow) {
        app->overflow_lines++;
    }
    app->line_len = 0;
    app->line_overflow = false;
}

static void ns_consume(NsApp* app, const uint8_t* bytes, size_t count) {
    for(size_t i = 0; i < count; i++) {
        char c = (char)bytes[i];
        app->ever_rx = true;
        app->rx_since_request = true;
        if(c == '\r') continue;
        if(c == '\n') ns_finish_line(app);
        else if(!app->line_overflow) {
            if((size_t)app->line_len + 1 < sizeof(app->line)) app->line[app->line_len++] = c;
            else app->line_overflow = true;
        }
    }
}

static void ns_serial_rx(FuriHalSerialHandle* handle, FuriHalSerialRxEvent event, void* ctx) {
    if(!(event & FuriHalSerialRxEventData)) return;
    NsApp* app = ctx;
    while(furi_hal_serial_async_rx_available(handle)) {
        uint8_t byte = furi_hal_serial_async_rx(handle);
        if(furi_stream_buffer_send(app->rx, &byte, 1, 0) == 0) app->dropped_bytes++;
    }
}

static bool ns_write(File* file, const char* str) {
    size_t n = strlen(str);
    return storage_file_write(file, str, n) == n;
}

static bool ns_export_csv(NsApp* app) {
    if(app->scanning != ScanNone || app->ap_count == 0) {
        ns_note(app, "Stop scan; collect APs");
        return false;
    }
    Storage* storage = furi_record_open(RECORD_STORAGE);
    storage_common_mkdir(storage, "/ext/apps_data");
    storage_common_mkdir(storage, NS_EXPORT_DIR);
    File* file = storage_file_alloc(storage);
    bool ok = false;
    for(unsigned i = 1; i <= 999; i++) {
        char path[88];
        snprintf(app->export_name, sizeof(app->export_name), "scan_%03u.csv", i);
        snprintf(path, sizeof(path), "%s/%s", NS_EXPORT_DIR, app->export_name);
        if(storage_file_exists(storage, path)) continue;
        if(storage_file_open(file, path, FSAM_WRITE, FSOM_CREATE_NEW)) {
            ok = ns_write(file, "ssid,bssid,channel,rssi_dbm\n");
            for(size_t j = 0; j < app->ap_count && ok; j++) {
                char record[128];
                if(!ns_csv_ap(&app->aps[j], record, sizeof(record))) ok = false;
                else ok = ns_write(file, record);
            }
            if(!storage_file_sync(file)) ok = false;
        }
        storage_file_close(file);
        break;
    }
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    ns_note(app, ok ? "CSV saved to Flipper SD" : "CSV write failed / SD?");
    return ok;
}

static void ns_baseline_save(NsApp* app) {
    if(app->scanning != ScanNone || app->ap_count == 0) {
        ns_note(app, "Stop Wi-Fi scan first");
        return;
    }
    memcpy(app->baseline, app->aps, sizeof(NsAccessPoint) * app->ap_count);
    app->baseline_count = app->ap_count;
    app->has_baseline = true;
    ns_note(app, "Baseline set (RAM)");
}

static void ns_input_callback(InputEvent* event, void* ctx) {
    NsApp* app = ctx;
    if(event->type == InputTypeShort || event->type == InputTypeRepeat) {
        furi_message_queue_put(app->inputs, event, 0);
    }
}

static void ns_line(Canvas* canvas, int y, const char* str) {
    canvas_draw_str(canvas, 2, y, str);
}

static void ns_draw_recent(Canvas* canvas, const NsApp* app, int first_y, uint8_t max_lines) {
    uint8_t count = max_lines < NS_RAW_ROWS ? max_lines : NS_RAW_ROWS;
    for(uint8_t i = 0; i < count; i++) {
        size_t idx = (app->raw_next + NS_RAW_ROWS - count + i) % NS_RAW_ROWS;
        canvas_draw_str(canvas, 2, first_y + i * 10, app->raw[idx]);
    }
}

static const char* ns_status(const NsApp* app) {
    if(!app->serial) return "UART BUSY";
    if(app->scanning != ScanNone) return "SCANNING";
    if(app->requested && !app->rx_since_request &&
       (uint32_t)(furi_get_tick() - app->last_request_tick) > furi_ms_to_ticks(2500)) {
        return "NO REPLY";
    }
    return app->ever_rx ? "RX OK" : "READY";
}

static void ns_draw(Canvas* canvas, void* ctx) {
    NsApp* app = ctx;
    furi_mutex_acquire(app->mutex, FuriWaitForever);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "NS Ultra v0.2");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 127, 10, AlignRight, AlignBottom, ns_status(app));
    canvas_draw_line(canvas, 0, 13, 127, 13);
    char temp[52];
    switch(app->page) {
    case PageMenu: {
        size_t first = app->menu_index < 4 ? 0 : (app->menu_index / 4) * 4;
        for(size_t i = first; i < first + 4 && i < NS_MENU_ITEMS; i++) {
            snprintf(temp, sizeof(temp), "%c %s", app->menu_index == i ? '>' : ' ', ns_menu[i]);
            ns_line(canvas, 24 + (int)(i - first) * 10, temp);
        }
        ns_line(canvas, 63, "Up/Down  OK Open");
        break;
    }
    case PageWifi: {
        snprintf(temp, sizeof(temp), "Wi-Fi APs: %u/%u", app->ap_count, NS_MAX_APS);
        ns_line(canvas, 23, temp);
        if(!app->ap_count) {
            ns_line(canvas, 36, app->scanning == ScanWifi ? "Scanning 2.4GHz ..." : "OK: Start AP scan");
            ns_line(canvas, 47, "Back: Main menu");
        } else {
            size_t first = app->wifi_index / 3 * 3;
            for(size_t i = first; i < app->ap_count && i < first + 3; i++) {
                const NsAccessPoint* ap = &app->aps[i];
                snprintf(temp, sizeof(temp), "%c %.15s", app->wifi_index == i ? '>' : ' ', ap->ssid);
                ns_line(canvas, 33 + 10 * (int)(i - first), temp);
                snprintf(temp, sizeof(temp), "%d", ap->rssi);
                canvas_draw_str_aligned(canvas, 127, 33 + 10 * (int)(i - first), AlignRight, AlignBottom, temp);
            }
        }
        ns_line(canvas, 63, "OK Scan/Stop  > Info");
        break;
    }
    case PageWifiDetail: {
        if(app->ap_count) {
            const NsAccessPoint* ap = &app->aps[app->wifi_index];
            ns_line(canvas, 23, "Access Point:");
            snprintf(temp, sizeof(temp), "%.22s", ap->ssid);
            ns_line(canvas, 33, temp);
            snprintf(temp, sizeof(temp), "Channel %u  RSSI %d", ap->channel, ap->rssi);
            ns_line(canvas, 43, temp);
            ns_line(canvas, 53, ap->bssid[0] ? ap->bssid : "BSSID not available");
        }
        ns_line(canvas, 63, "Up/Down AP  Back List");
        break;
    }
    case PageChannel: {
        uint8_t counts[14];
        ns_channel_counts(app->aps, app->ap_count, counts);
        ns_line(canvas, 23, "Observed APs per channel");
        snprintf(temp, sizeof(temp), "Ch 1: %u   Ch 6: %u   Ch 11: %u", counts[0], counts[5], counts[10]);
        ns_line(canvas, 34, temp);
        snprintf(temp, sizeof(temp), "> Ch %u: %u APs", app->channel_index + 1, counts[app->channel_index]);
        ns_line(canvas, 45, temp);
        ns_line(canvas, 54, "Not spectrum occupancy");
        ns_line(canvas, 63, "Up/Down Ch  OK Scan");
        break;
    }
    case PageClients:
    case PageBluetooth:
        ns_line(canvas, 23, app->page == PageClients ? "Client serial observations" : "Bluetooth serial monitor");
        ns_draw_recent(canvas, app, 34, 2);
        snprintf(temp, sizeof(temp), "RX lines: %lu", (unsigned long)app->serial_lines);
        ns_line(canvas, 54, temp);
        ns_line(canvas, 63, "OK Start/Stop  Back");
        break;
    case PageBaseline: {
        if(!app->has_baseline) ns_line(canvas, 24, "No baseline saved in RAM");
        else {
            NsDiff diff = ns_diff(app->baseline, app->baseline_count, app->aps, app->ap_count);
            snprintf(temp, sizeof(temp), "Before %u / Now %u", app->baseline_count, app->ap_count);
            ns_line(canvas, 24, temp);
            snprintf(temp, sizeof(temp), "New %u  Missing %u", diff.new_aps, diff.missing_aps);
            ns_line(canvas, 35, temp);
            snprintf(temp, sizeof(temp), "Channel changes: %u", diff.channel_changes);
            ns_line(canvas, 46, temp);
            ns_line(canvas, 54, "Missing != disappeared");
        }
        ns_line(canvas, 63, "OK Save baseline  Back");
        break;
    }
    case PageExport:
        ns_line(canvas, 24, "Export last Wi-Fi list");
        snprintf(temp, sizeof(temp), "Recorded: %u APs", app->ap_count);
        ns_line(canvas, 35, temp);
        if(app->export_ok) ns_line(canvas, 46, app->export_name);
        else ns_line(canvas, 46, app->notice[0] ? app->notice : "Save into apps_data");
        ns_line(canvas, 63, "OK Save CSV  Back");
        break;
    case PageInfo:
        ns_line(canvas, 24, "ESP32 Marauder info");
        ns_draw_recent(canvas, app, 34, 2);
        ns_line(canvas, 63, "OK Request info  Back");
        break;
    case PageRaw:
        ns_draw_recent(canvas, app, 23, 4);
        snprintf(temp, sizeof(temp), "Lines %lu Drop %lu Ov %u", (unsigned long)app->serial_lines,
                 (unsigned long)app->dropped_bytes, app->overflow_lines);
        ns_line(canvas, 63, temp);
        break;
    case PageHardware:
        if(app->hardware_index == 0) {
            ns_line(canvas, 24, "ESP32: Wi-Fi / Bluetooth");
            ns_line(canvas, 35, "2.4GHz, Marauder UART");
            ns_line(canvas, 46, "Powered in either mode");
        } else if(app->hardware_index == 1) {
            ns_line(canvas, 24, "NRF24: 2.4GHz radio");
            ns_line(canvas, 35, "Set physical NRF switch");
            ns_line(canvas, 46, "Driver not in this FAP");
        } else {
            ns_line(canvas, 24, "CC1101: Sub-GHz radio");
            ns_line(canvas, 35, "Set physical CC1101 switch");
            ns_line(canvas, 46, "Use external SubGHz app");
        }
        ns_line(canvas, 63, "Up/Down module  Back");
        break;
    }
    furi_mutex_release(app->mutex);
}

static void ns_open_menu(NsApp* app) {
    switch(app->menu_index) {
    case 0: app->page = PageWifi; break;
    case 1: app->page = PageChannel; break;
    case 2: app->page = PageClients; break;
    case 3: app->page = PageBluetooth; break;
    case 4: app->page = PageBaseline; break;
    case 5: app->page = PageExport; break;
    case 6: app->page = PageInfo; ns_uart_send(app, "info\n"); break;
    case 7: app->page = PageRaw; break;
    case 8: app->page = PageHardware; break;
    }
}

static void ns_handle_key(NsApp* app, const InputEvent* event, bool* running) {
    InputKey key = event->key;
    NsPage page = app->page;
    if(event->type == InputTypeRepeat && key != InputKeyUp && key != InputKeyDown) return;
    if(key == InputKeyBack) {
        if(page == PageMenu) *running = false;
        else if(page == PageWifiDetail) app->page = PageWifi;
        else {
            if(app->scanning != ScanNone) ns_stop(app);
            app->page = PageMenu;
        }
        return;
    }
    if(page == PageMenu) {
        if(key == InputKeyDown && app->menu_index + 1 < NS_MENU_ITEMS) app->menu_index++;
        else if(key == InputKeyUp && app->menu_index) app->menu_index--;
        else if(key == InputKeyOk && event->type == InputTypeShort) ns_open_menu(app);
    } else if(page == PageWifi || page == PageWifiDetail) {
        if(key == InputKeyDown && app->ap_count && app->wifi_index + 1 < app->ap_count) app->wifi_index++;
        else if(key == InputKeyUp && app->wifi_index) app->wifi_index--;
        else if(key == InputKeyRight && page == PageWifi && app->ap_count) app->page = PageWifiDetail;
        else if(key == InputKeyOk && page == PageWifi && event->type == InputTypeShort) ns_toggle(app, ScanWifi);
    } else if(page == PageChannel) {
        if(key == InputKeyDown && app->channel_index < 13) app->channel_index++;
        else if(key == InputKeyUp && app->channel_index) app->channel_index--;
        else if(key == InputKeyOk && event->type == InputTypeShort) ns_toggle(app, ScanWifi);
    } else if(page == PageClients || page == PageBluetooth) {
        if(key == InputKeyOk && event->type == InputTypeShort)
            ns_toggle(app, page == PageClients ? ScanClients : ScanBluetooth);
    } else if(page == PageBaseline && key == InputKeyOk && event->type == InputTypeShort) {
        ns_baseline_save(app);
    } else if(page == PageExport && key == InputKeyOk && event->type == InputTypeShort) {
        app->export_ok = ns_export_csv(app);
    } else if(page == PageInfo && key == InputKeyOk && event->type == InputTypeShort) {
        ns_uart_send(app, "info\n");
    } else if(page == PageHardware) {
        if(key == InputKeyDown && app->hardware_index < 2) app->hardware_index++;
        else if(key == InputKeyUp && app->hardware_index) app->hardware_index--;
    }
}

int32_t netsentinel_app(void* p) {
    UNUSED(p);
    NsApp* app = malloc(sizeof(NsApp));
    if(!app) return 1;
    memset(app, 0, sizeof(*app));
    app->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    app->inputs = furi_message_queue_alloc(16, sizeof(InputEvent));
    app->rx = furi_stream_buffer_alloc(NS_RX_CAPACITY, 1);
    app->port = view_port_alloc();
    view_port_draw_callback_set(app->port, ns_draw, app);
    view_port_input_callback_set(app->port, ns_input_callback, app);
    app->gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(app->gui, app->port, GuiLayerFullscreen);

    Expansion* expansion = furi_record_open(RECORD_EXPANSION);
    expansion_disable(expansion);
    bool otg_original = furi_hal_power_is_otg_enabled();
    if(!otg_original) {
        for(int tries = 0; tries < 5 && !furi_hal_power_is_otg_enabled(); tries++) {
            furi_hal_power_enable_otg();
            furi_delay_ms(30);
        }
    }
    app->serial = furi_hal_serial_control_acquire(FuriHalSerialIdUsart);
    if(app->serial) {
        furi_hal_serial_init(app->serial, NS_SERIAL_BAUD);
        furi_hal_serial_async_rx_start(app->serial, ns_serial_rx, app, false);
    }
    bool running = true;
    view_port_update(app->port);
    while(running) {
        bool redraw = false;
        uint8_t buf[256];
        for(size_t i = 0; i < 16; i++) {
            size_t size = furi_stream_buffer_receive(app->rx, buf, sizeof(buf), 0);
            if(!size) break;
            furi_mutex_acquire(app->mutex, FuriWaitForever);
            ns_consume(app, buf, size);
            furi_mutex_release(app->mutex);
            redraw = true;
        }
        if(app->listing_pending != ScanNone &&
           (int32_t)(furi_get_tick() - app->list_at_tick) >= 0) {
            NsScan pending = app->listing_pending;
            app->listing_pending = ScanNone;
            ns_uart_send(app, pending == ScanWifi ? "list -a\n" : "list -c\n");
            redraw = true;
        }
        InputEvent ev;
        if(furi_message_queue_get(app->inputs, &ev, 55) == FuriStatusOk) {
            /* UI and input thread state is protected by same mutex. */
            furi_mutex_acquire(app->mutex, FuriWaitForever);
            ns_handle_key(app, &ev, &running);
            furi_mutex_release(app->mutex);
            redraw = true;
        }
        if(redraw || app->requested) view_port_update(app->port);
    }
    if(app->serial) {
        if(app->scanning != ScanNone) ns_uart_send(app, "stopscan\n");
        furi_hal_serial_async_rx_stop(app->serial);
        furi_hal_serial_deinit(app->serial);
        furi_hal_serial_control_release(app->serial);
    }
    expansion_enable(expansion);
    furi_record_close(RECORD_EXPANSION);
    if(!otg_original && furi_hal_power_is_otg_enabled()) furi_hal_power_disable_otg();
    gui_remove_view_port(app->gui, app->port);
    furi_record_close(RECORD_GUI);
    view_port_free(app->port);
    furi_stream_buffer_free(app->rx);
    furi_message_queue_free(app->inputs);
    furi_mutex_free(app->mutex);
    free(app);
    return 0;
}
