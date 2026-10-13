#include "mock_sdk.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
struct FuriMutex { bool locked; };
struct FuriMessageQueue { size_t item_size; };
struct Canvas { unsigned calls; };
struct ViewPort { void (*draw)(Canvas*, void*); void* draw_ctx; bool enabled, attached; };
struct Gui { unsigned unused; };
struct Storage { unsigned unused; };
typedef struct { char path[100]; uint8_t data[8192]; size_t size; bool exists; } Entry;
struct File { Entry* entry; size_t position; FS_Error error; FS_AccessMode mode; };
static Entry files[16];
static unsigned live, records, allocations, fail_allocation, draws;
static bool fail_storage;
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
void mock_sdk_reset(void) { assert(!live && !records); memset(files, 0, sizeof(files)); allocations = fail_allocation = draws = 0; fail_storage = false; ticks = 0; event_count = event_index = 0; }
void mock_sdk_events(const InputEvent* e, size_t n) { events = e; event_count = n; event_index = 0; }
void mock_sdk_fail_allocation(unsigned n) { fail_allocation = n; }
void mock_sdk_fail_storage(bool fail) { fail_storage = fail; }
unsigned mock_sdk_live(void) { return live; }
unsigned mock_sdk_records(void) { return records; }
unsigned mock_sdk_draws(void) { return draws; }
FuriMutex* furi_mutex_alloc(FuriMutexType t) { (void)t; return allocate(sizeof(FuriMutex)); }
void furi_mutex_free(FuriMutex* m) { assert(!m->locked); dispose(m); }
FuriStatus furi_mutex_acquire(FuriMutex* m, uint32_t wait) { (void)wait; assert(!m->locked); m->locked = true; return FuriStatusOk; }
void furi_mutex_release(FuriMutex* m) { assert(m->locked); m->locked = false; }
FuriMessageQueue* furi_message_queue_alloc(size_t n, size_t size) { (void)n; FuriMessageQueue* q = allocate(sizeof(*q)); if(q) q->item_size = size; return q; }
void furi_message_queue_free(FuriMessageQueue* q) { dispose(q); }
FuriStatus furi_message_queue_put(FuriMessageQueue* q, const void* data, uint32_t wait) { (void)q; (void)data; (void)wait; return FuriStatusOk; }
FuriStatus furi_message_queue_get(FuriMessageQueue* q, void* out, uint32_t wait) {
    assert(q->item_size == sizeof(InputEvent)); ticks += wait;
    if(event_index < event_count) memcpy(out, events+event_index++, sizeof(InputEvent));
    else *(InputEvent*)out = (InputEvent){InputKeyBack, InputTypeShort};
    return FuriStatusOk;
}
void* furi_record_open(const char* name) { records++; return !strcmp(name, RECORD_GUI) ? (void*)&gui : (void*)&storage; }
void furi_record_close(const char* name) { (void)name; assert(records); records--; }
uint32_t furi_get_tick(void) { return ticks; }
uint32_t furi_ms_to_ticks(uint32_t ms) { return ms; }
ViewPort* view_port_alloc(void) { ViewPort* p = allocate(sizeof(*p)); if(p) p->enabled = true; return p; }
void view_port_free(ViewPort* p) { assert(!p->attached); dispose(p); }
void view_port_draw_callback_set(ViewPort* p, void (*cb)(Canvas*, void*), void* ctx) { p->draw = cb; p->draw_ctx = ctx; }
void view_port_input_callback_set(ViewPort* p, void (*cb)(InputEvent*, void*), void* ctx) { (void)p; (void)cb; (void)ctx; }
void view_port_update(ViewPort* p) { if(p->enabled && p->draw) { Canvas c = {0}; p->draw(&c, p->draw_ctx); draws++; } }
void view_port_enabled_set(ViewPort* p, bool on) { p->enabled = on; }
void gui_add_view_port(Gui* g, ViewPort* p, GuiLayer layer) { (void)g; (void)layer; assert(!p->attached); p->attached = true; }
void gui_remove_view_port(Gui* g, ViewPort* p) { (void)g; assert(p->attached); p->attached = false; }
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
