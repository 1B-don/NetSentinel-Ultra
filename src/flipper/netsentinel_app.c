#include "ns_flipper.h"
#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>
typedef struct {
    NsApplication app;
    NsFlipper port;
    NsFrame frame;
    FuriMutex* mutex;
    FuriMessageQueue* inputs;
    ViewPort* viewport;
    Gui* gui;
    atomic_bool back_requested;
} NsGui;
static void draw(Canvas* canvas, void* ctx) {
    NsGui* g = ctx;
    if(furi_mutex_acquire(g->mutex, FuriWaitForever) != FuriStatusOk) return;
    canvas_clear(canvas); canvas_set_font(canvas, FontSecondary);
    for(unsigned i = 0; i < 6; i++) canvas_draw_str(canvas, 1, 10+i*10, g->frame.rows[i]);
    for(size_t i = 0; i < g->frame.bar_count; i++) {
        if(g->frame.bars[i]) {
            int x = g->frame.cc_plot ? (int)i*2+1 : (int)i+1;
            int baseline = g->frame.cc_plot ? 41 : 42;
            canvas_draw_line(canvas, x, baseline, x, baseline-g->frame.bars[i]);
        }
    }
    furi_mutex_release(g->mutex);
}
static void input(InputEvent* event, void* ctx) {
    NsGui* g = ctx;
    if(event->key == InputKeyBack && (event->type == InputTypeShort || event->type == InputTypeLong)) {
        atomic_store(&g->back_requested, true); return;
    }
    if(event->type == InputTypeShort || (event->type == InputTypeRepeat &&
       (event->key == InputKeyUp || event->key == InputKeyDown)))
        (void)furi_message_queue_put(g->inputs, event, 0);
}
static bool key(InputKey k, NsKey* out) {
    switch(k) {
    case InputKeyUp: *out = NS_KEY_UP; break;
    case InputKeyDown: *out = NS_KEY_DOWN; break;
    case InputKeyLeft: *out = NS_KEY_LEFT; break;
    case InputKeyRight: *out = NS_KEY_RIGHT; break;
    case InputKeyOk: *out = NS_KEY_OK; break;
    case InputKeyBack: *out = NS_KEY_BACK; break;
    default: return false;
    }
    return true;
}
int32_t netsentinel_app(void* context) {
    UNUSED(context);
    NsGui* g = calloc(1, sizeof(*g)); if(!g) return 1;
    atomic_init(&g->back_requested, false);
    bool port_ready = false, app_ready = false, attached = false;
    uint32_t now_ms = 0;
    int32_t result = 1;
    g->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    g->inputs = furi_message_queue_alloc(16, sizeof(InputEvent));
    g->viewport = view_port_alloc();
    if(!g->mutex || !g->inputs || !g->viewport) goto cleanup;
    if(!ns_flipper_init(&g->port)) goto cleanup;
    port_ready = true;
    (void)ns_app_init(&g->app, &g->port.platform, &g->port.store, g->port.capabilities);
    app_ready = true;
    ns_app_frame(&g->app, &g->frame);
    view_port_draw_callback_set(g->viewport, draw, g);
    view_port_input_callback_set(g->viewport, input, g);
    g->gui = furi_record_open(RECORD_GUI); if(!g->gui) goto cleanup;
    gui_add_view_port(g->gui, g->viewport, GuiLayerFullscreen); attached = true;
    uint32_t previous_tick = furi_get_tick();
    uint64_t fractional = 0;
    uint32_t ticks_per_second = furi_ms_to_ticks(1000);
    if(!ticks_per_second) goto cleanup;
    while(g->app.running) {
        InputEvent event;
        bool received = furi_message_queue_get(g->inputs, &event, furi_ms_to_ticks(20)) == FuriStatusOk;
        uint32_t tick = furi_get_tick();
        fractional += (uint64_t)(uint32_t)(tick-previous_tick)*1000;
        now_ms += (uint32_t)(fractional/ticks_per_second);
        fractional %= ticks_per_second; previous_tick = tick;
        /* The application and all I/O belong to this worker. Draw only reads
         * the published frame, and never holds the mutex during I/O. */
        NsKey translated;
        if(atomic_exchange(&g->back_requested, false)) (void)ns_app_key(&g->app, NS_KEY_BACK, now_ms);
        else if(received && key(event.key, &translated)) (void)ns_app_key(&g->app, translated, now_ms);
        if(g->app.running) (void)ns_app_step(&g->app, now_ms);
        if(furi_mutex_acquire(g->mutex, FuriWaitForever) != FuriStatusOk) {
            g->app.session.error = NS_IO; goto cleanup;
        }
        ns_app_frame(&g->app, &g->frame);
        furi_mutex_release(g->mutex);
        view_port_update(g->viewport);
    }
    result = g->app.status == NS_OK ? 0 : 1;
cleanup:
    /* Remove callbacks before session/storage teardown. Do not hold the frame
     * mutex across viewport calls: the GUI can acquire it from draw(). */
    if(g->viewport) {
        view_port_input_callback_set(g->viewport, NULL, NULL);
        view_port_draw_callback_set(g->viewport, NULL, NULL);
        if(attached) {
            view_port_enabled_set(g->viewport, false);
            gui_remove_view_port(g->gui, g->viewport);
        }
    }
    if(app_ready) {
        /* Abnormal worker failure cannot await a GUI-driven handshake. Force
         * bounded best-effort stop/release, then persist completed history. */
        if(g->app.session.active != NS_RADIO_NONE) {
            ns_session_stop(&g->app.session, now_ms); g->app.history_dirty = true;
        }
        (void)ns_app_stop(&g->app, now_ms);
    }
    if(g->gui) furi_record_close(RECORD_GUI);
    if(g->viewport) view_port_free(g->viewport);
    if(port_ready) ns_flipper_deinit(&g->port);
    if(g->inputs) furi_message_queue_free(g->inputs);
    if(g->mutex) furi_mutex_free(g->mutex);
    free(g); return result;
}
