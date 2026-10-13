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
} NsGui;
static void draw(Canvas* canvas, void* ctx) {
    NsGui* g = ctx;
    if(furi_mutex_acquire(g->mutex, FuriWaitForever) != FuriStatusOk) return;
    canvas_clear(canvas); canvas_set_font(canvas, FontSecondary);
    for(unsigned i = 0; i < 6; i++) canvas_draw_str(canvas, 1, 10+i*10, g->frame.rows[i]);
    for(size_t i = 0; i < g->frame.bar_count; i++) {
        if(g->frame.bars[i]) canvas_draw_line(canvas, (int)i+1, 42, (int)i+1, 42-g->frame.bars[i]);
    }
    furi_mutex_release(g->mutex);
}
static void input(InputEvent* event, void* ctx) {
    NsGui* g = ctx;
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
    bool port_ready = false, attached = false;
    int32_t result = 1;
    g->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    g->inputs = furi_message_queue_alloc(16, sizeof(InputEvent));
    g->viewport = view_port_alloc();
    if(!g->mutex || !g->inputs || !g->viewport) goto cleanup;
    if(!ns_flipper_init(&g->port)) goto cleanup;
    port_ready = true;
    (void)ns_app_init(&g->app, &g->port.platform, &g->port.store, g->port.capabilities);
    ns_app_frame(&g->app, &g->frame);
    view_port_draw_callback_set(g->viewport, draw, g);
    view_port_input_callback_set(g->viewport, input, g);
    g->gui = furi_record_open(RECORD_GUI); if(!g->gui) goto cleanup;
    gui_add_view_port(g->gui, g->viewport, GuiLayerFullscreen); attached = true;
    uint32_t previous_tick = furi_get_tick(), now_ms = 0;
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
        if(received && key(event.key, &translated)) (void)ns_app_key(&g->app, translated, now_ms);
        if(g->app.running) (void)ns_app_step(&g->app, now_ms);
        furi_mutex_acquire(g->mutex, FuriWaitForever);
        ns_app_frame(&g->app, &g->frame);
        furi_mutex_release(g->mutex);
        view_port_update(g->viewport);
    }
    result = g->app.status == NS_OK ? 0 : 1;
cleanup:
    if(port_ready) (void)ns_app_stop(&g->app, 0);
    if(attached) { view_port_enabled_set(g->viewport, false); gui_remove_view_port(g->gui, g->viewport); }
    if(g->gui) furi_record_close(RECORD_GUI);
    if(g->viewport) view_port_free(g->viewport);
    if(port_ready) ns_flipper_deinit(&g->port);
    if(g->inputs) furi_message_queue_free(g->inputs);
    if(g->mutex) furi_mutex_free(g->mutex);
    free(g); return result;
}
