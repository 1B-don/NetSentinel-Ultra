#include "mock_sdk.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
struct FuriMutex { bool locked; };
struct FuriMessageQueue { size_t item_size, capacity, first, count; InputEvent data[16]; };
struct Canvas { unsigned calls; };
struct ViewPort { void (*draw)(Canvas*, void*); void* draw_ctx; void (*input)(InputEvent*, void*); void* input_ctx; bool enabled, attached; };
struct Gui { unsigned unused; };
struct Storage { unsigned unused; };
typedef struct { char path[100]; uint8_t data[8192]; size_t size; bool exists; } Entry;
struct File { Entry* entry; size_t position; FS_Error error; FS_AccessMode mode; };
static Entry files[16];
static unsigned live, records, allocations, fail_allocation, draws;
static bool fail_storage, fail_queue, burst;
static const char* failed_record;
static unsigned mutex_calls, mutex_failure, queue_drops;
static ViewPort* active_viewport;
static uint32_t ticks;
static const InputEvent* events;
static size_t event_count, event_index;
static struct Gui gui;
static struct Storage storage;
static void* allocate(size_t size) {
    allocations++; if(allocations == fail_allocation) return NULL;
    void* p = calloc(1, size); if(p) live++; return p;
}
static void dispose(void* p) { assert(p && live); live--; free(p); }
void mock_sdk_reset(void) { assert(!live && !records); memset(files, 0, sizeof(files)); allocations = fail_allocation = draws = 0; fail_storage = fail_queue = burst = false; failed_record = NULL; mutex_calls = mutex_failure = queue_drops = 0;
    assert(!active_viewport); ticks = 0; event_count = event_index = 0; }
void mock_sdk_events(const InputEvent* e, size_t n) { events = e; event_count = n; event_index = 0; }
void mock_sdk_fail_allocation(unsigned n) { fail_allocation = n; }
void mock_sdk_fail_storage(bool fail) { fail_storage = fail; }
void mock_sdk_fail_record(const char* name) { failed_record = name; }
void mock_sdk_fail_mutex(unsigned call) { mutex_failure = call; }
void mock_sdk_fail_queue(bool on) { fail_queue = on; }
void mock_sdk_burst(bool on) { burst = on; }
unsigned mock_sdk_queue_drops(void) { return queue_drops; }
unsigned mock_sdk_live(void) { return live; }
unsigned mock_sdk_records(void) { return records; }
unsigned mock_sdk_draws(void) { return draws; }
FuriMutex* furi_mutex_alloc(FuriMutexType t) { (void)t; return allocate(sizeof(FuriMutex)); }
void furi_mutex_free(FuriMutex* m) { assert(!m->locked); dispose(m); }
FuriStatus furi_mutex_acquire(FuriMutex* m, uint32_t wait) { (void)wait; mutex_calls++; if(mutex_calls == mutex_failure) return FuriStatusError;
    assert(!m->locked); m->locked = true; return FuriStatusOk; }
void furi_mutex_release(FuriMutex* m) { assert(m->locked); m->locked = false; }
FuriMessageQueue* furi_message_queue_alloc(size_t n, size_t size) { assert(n <= 16 && size == sizeof(InputEvent)); FuriMessageQueue* q = allocate(sizeof(*q));
    if(q) { q->item_size = size; q->capacity = n; } return q; }
void furi_message_queue_free(FuriMessageQueue* q) { dispose(q); }
FuriStatus furi_message_queue_put(FuriMessageQueue* q, const void* data, uint32_t wait) { (void)wait;
    if(fail_queue || q->count == q->capacity) { queue_drops++; return FuriStatusError; }
    memcpy(q->data+(q->first+q->count)%q->capacity, data, q->item_size); q->count++;
    return FuriStatusOk; }
FuriStatus furi_message_queue_get(FuriMessageQueue* q, void* out, uint32_t wait) {
    assert(q->item_size == sizeof(InputEvent)); ticks += wait;
    assert(active_viewport && active_viewport->input);
    do {
        InputEvent e = event_index < event_count ? events[event_index++] : (InputEvent){InputKeyBack, InputTypeShort};
        active_viewport->input(&e, active_viewport->input_ctx);
    } while(burst && event_index < event_count);
    if(!q->count) return FuriStatusError;
    memcpy(out, q->data+q->first, q->item_size); q->first = (q->first+1)%q->capacity; q->count--;
    return FuriStatusOk;
}
void* furi_record_open(const char* name) { if(failed_record && !strcmp(name, failed_record)) return NULL;
    records++; return !strcmp(name, RECORD_GUI) ? (void*)&gui : (void*)&storage; }
void furi_record_close(const char* name) { (void)name; assert(records); records--; }
uint32_t furi_get_tick(void) { return ticks; }
uint32_t furi_ms_to_ticks(uint32_t ms) { return ms; }
ViewPort* view_port_alloc(void) { ViewPort* p = allocate(sizeof(*p)); if(p) p->enabled = true; return p; }
void view_port_free(ViewPort* p) { assert(!p->attached && !p->draw && !p->input); dispose(p); }
void view_port_draw_callback_set(ViewPort* p, void (*cb)(Canvas*, void*), void* ctx) { p->draw = cb; p->draw_ctx = ctx; }
void view_port_input_callback_set(ViewPort* p, void (*cb)(InputEvent*, void*), void* ctx) { p->input = cb; p->input_ctx = ctx; }
void view_port_update(ViewPort* p) { if(p->enabled && p->draw) { Canvas c = {0}; p->draw(&c, p->draw_ctx); draws++; } }
void view_port_enabled_set(ViewPort* p, bool on) { p->enabled = on; }
void gui_add_view_port(Gui* g, ViewPort* p, GuiLayer layer) { (void)g; (void)layer; assert(!p->attached); p->attached = true; active_viewport = p; }
void gui_remove_view_port(Gui* g, ViewPort* p) { (void)g; assert(p->attached); p->attached = false; active_viewport = NULL; }
void canvas_clear(Canvas* c) { c->calls++; }
void canvas_set_font(Canvas* c, Font f) { (void)c; (void)f; }
void canvas_draw_str(Canvas* c, int x, int y, const char* s) { (void)c; assert(x >= 0 && x < 128 && y > 0 && y <= 64 && s); }
void canvas_draw_line(Canvas* c, int x1, int y1, int x2, int y2) { (void)c; assert(x1 >= 0 && x1 < 128 && x2 >= 0 && x2 < 128 && y1 >= 0 && y1 < 64 && y2 >= 0 && y2 < 64); }
File* storage_file_alloc(Storage* s) { (void)s; return allocate(sizeof(File)); }
void storage_file_free(File* f) { assert(!f->entry); dispose(f); }
bool storage_file_open(File* f, const char* path, FS_AccessMode access, FS_OpenMode mode) {
    assert(!f->entry); f->error = FSE_OK;
    if(fail_storage) { f->error = FSE_ERROR; return false; }
    Entry* match = NULL; Entry* vacant = NULL;
    for(unsigned i = 0; i < 16; i++) {
        if(files[i].exists && !strcmp(files[i].path, path)) match = &files[i];
        if(!files[i].exists && !vacant) vacant = &files[i];
    }
    if(!match && mode == FSOM_OPEN_EXISTING) { f->error = FSE_NOT_EXIST; return false; }
    if(match && mode == FSOM_CREATE_NEW) { f->error = FSE_EXIST; return false; }
    if(!match) {
        if(!vacant) { f->error = FSE_ERROR; return false; }
        match = vacant; match->exists = true;
        assert(strlen(path) < sizeof(match->path)); strcpy(match->path, path);
    }
    if(mode == FSOM_CREATE_ALWAYS) match->size = 0;
    f->entry = match; f->position = 0; f->mode = access; return true;
}
bool storage_file_close(File* f) { f->entry = NULL; return true; }
size_t storage_file_read(File* f, void* data, size_t n) {
    assert(f->entry && f->mode == FSAM_READ);
    if(fail_storage) { f->error = FSE_ERROR; return 0; }
    if(n > f->entry->size-f->position) n = f->entry->size-f->position;
    memcpy(data, f->entry->data+f->position, n); f->position += n; return n;
}
size_t storage_file_write(File* f, const void* data, size_t n) {
    assert(f->entry && f->mode == FSAM_WRITE);
    if(fail_storage || n > sizeof(f->entry->data)-f->position) { f->error = FSE_ERROR; return 0; }
    memcpy(f->entry->data+f->position, data, n); f->position += n;
    if(f->position > f->entry->size) f->entry->size = f->position;
    return n;
}
uint64_t storage_file_size(File* f) { assert(f->entry); return f->entry->size; }
bool storage_file_sync(File* f) { assert(f->entry); return !fail_storage; }
FS_Error storage_file_get_error(File* f) { return f->error; }
FS_Error storage_common_mkdir(Storage* s, const char* path) { (void)s; assert(!strcmp(path, "/ext/apps_data") || !strcmp(path, "/ext/apps_data/netsentinel")); return fail_storage ? FSE_ERROR : FSE_OK; }

#include "furi_hal_serial_control.h"
struct FuriHalSerialHandle { bool owned, initialized, available; uint8_t byte; FuriHalSerialAsyncRxCallback callback; void* context; };
static struct FuriHalSerialHandle serial;
static char commands[1024];
FuriHalSerialHandle* furi_hal_serial_control_acquire(FuriHalSerialId id) {
    assert(id == FuriHalSerialIdUsart); if(serial.owned) return NULL;
    serial.owned = true; commands[0] = 0; return &serial;
}
void furi_hal_serial_control_release(FuriHalSerialHandle* s) { assert(s->owned && !s->initialized && !s->callback); s->owned = false; }
void furi_hal_serial_init(FuriHalSerialHandle* s, uint32_t baud) { assert(s->owned && baud == 115200); s->initialized = true; }
void furi_hal_serial_deinit(FuriHalSerialHandle* s) { assert(s->initialized && !s->callback); s->initialized = false; }
void furi_hal_serial_async_rx_start(FuriHalSerialHandle* s, FuriHalSerialAsyncRxCallback cb, void* ctx, bool errors) { assert(s->initialized && errors); s->callback = cb; s->context = ctx; }
void furi_hal_serial_async_rx_stop(FuriHalSerialHandle* s) { assert(s->initialized); s->callback = NULL; }
bool furi_hal_serial_async_rx_available(FuriHalSerialHandle* s) { assert(s->callback); return s->available; }
uint8_t furi_hal_serial_async_rx(FuriHalSerialHandle* s) { assert(s->callback && s->available); s->available = false; return s->byte; }
void furi_hal_serial_tx(FuriHalSerialHandle* s, const uint8_t* data, size_t count) {
    assert(s->initialized); size_t len = strlen(commands); assert(len+count < sizeof(commands)); memcpy(commands+len, data, count); commands[len+count] = 0;
}
void mock_serial_feed(const uint8_t* data, size_t count) { assert(serial.callback); for(size_t i=0; i<count; i++) { serial.byte=data[i]; serial.available=true; serial.callback(&serial, FuriHalSerialRxEventData, serial.context); } }
void mock_serial_error(FuriHalSerialRxEvent e) { assert(serial.callback); serial.callback(&serial, e, serial.context); }
const char* mock_serial_commands(void) { return commands; }

static bool wrong_pins;
void mock_serial_pin_mismatch(bool wrong) { wrong_pins = wrong; }
const GpioPin* furi_hal_serial_get_gpio_pin(FuriHalSerialHandle* s, FuriHalSerialDirection d) {
    assert(s->owned && !s->initialized);
    static const GpioPin tx = {13}, rx = {14};
    return (d == FuriHalSerialDirectionTx) != wrong_pins ? &tx : &rx;
}
int32_t furi_hal_resources_get_ext_pin_number(const GpioPin* pin) { return pin ? pin->connector : -1; }
bool furi_hal_serial_is_baud_rate_supported(FuriHalSerialHandle* s, uint32_t baud) {
    assert(s->owned); return baud == 115200;
}
void furi_hal_serial_configure_framing(FuriHalSerialHandle* s, FuriHalSerialDataBits bits,
    FuriHalSerialParity parity, FuriHalSerialStopBits stop) {
    assert(s->initialized && bits == FuriHalSerialDataBits8 && parity == FuriHalSerialParityNone && stop == FuriHalSerialStopBits1);
}
