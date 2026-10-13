#include "ns_flipper.h"
#include <furi.h>
#include <string.h>
#define NS_DIRECTORY "/ext/apps_data/netsentinel"
static const char* const slots[] = {NS_DIRECTORY "/state_0.bin", NS_DIRECTORY "/state_1.bin"};
static NsResult unavailable(void* ctx, NsRadio radio) { (void)ctx; (void)radio; return NS_UNSUPPORTED; }
static void no_release(void* ctx, NsRadio radio) { (void)ctx; (void)radio; }
static NsResult prepare(NsFlipper* f) {
    FS_Error r = storage_common_mkdir(f->storage, "/ext/apps_data");
    if(r != FSE_OK && r != FSE_EXIST) return NS_IO;
    r = storage_common_mkdir(f->storage, NS_DIRECTORY);
    return r == FSE_OK || r == FSE_EXIST ? NS_OK : NS_IO;
}
static NsResult slot_read(void* ctx, unsigned slot, uint8_t* out, size_t capacity, size_t* length) {
    NsFlipper* f = ctx;
    if(slot > 1 || !out || !length) return NS_INVALID;
    File* file = storage_file_alloc(f->storage); if(!file) return NS_IO;
    NsResult r = NS_IO; *length = 0;
    if(storage_file_open(file, slots[slot], FSAM_READ, FSOM_OPEN_EXISTING)) {
        uint64_t size = storage_file_size(file);
        if(size > capacity) r = NS_FULL;
        else {
            *length = storage_file_read(file, out, (size_t)size);
            if(*length == size && storage_file_get_error(file) == FSE_OK) r = NS_OK;
        }
    } else if(storage_file_get_error(file) == FSE_NOT_EXIST) r = NS_ABSENT;
    if(!storage_file_close(file) && r == NS_OK) r = NS_IO;
    storage_file_free(file); return r;
}
static NsResult slot_write(void* ctx, unsigned slot, const uint8_t* data, size_t length) {
    NsFlipper* f = ctx;
    if(slot > 1 || !data || length > NS_STATE_CAPACITY) return NS_INVALID;
    NsResult r = prepare(f); if(r != NS_OK) return r;
    File* file = storage_file_alloc(f->storage); if(!file) return NS_IO;
    r = NS_IO;
    if(storage_file_open(file, slots[slot], FSAM_WRITE, FSOM_CREATE_ALWAYS) &&
       storage_file_write(file, data, length) == length && storage_file_sync(file)) r = NS_OK;
    if(!storage_file_close(file) && r == NS_OK) r = NS_IO;
    storage_file_free(file); return r;
}
static NsResult log_create(void* ctx, const char* path) {
    NsFlipper* f = ctx;
    if(f->log_open) return NS_BUSY;
    const char* prefix = NS_DIRECTORY "/session_";
    size_t len = strlen(prefix);
    if(strncmp(path, prefix, len) || strlen(path+len) != 14 || strcmp(path+len+10, ".csv")) return NS_INVALID;
    for(unsigned i = 0; i < 10; i++) if(path[len+i] < '0' || path[len+i] > '9') return NS_INVALID;
    NsResult r = prepare(f); if(r != NS_OK) return r;
    if(storage_file_open(f->log, path, FSAM_WRITE, FSOM_CREATE_NEW)) { f->log_open = true; return NS_OK; }
    r = storage_file_get_error(f->log) == FSE_EXIST ? NS_BUSY : NS_IO;
    (void)storage_file_close(f->log); return r;
}
static NsResult log_write(void* ctx, const char* data, size_t length) {
    NsFlipper* f = ctx;
    return f->log_open && storage_file_write(f->log, data, length) == length ? NS_OK : NS_IO;
}
static NsResult log_sync(void* ctx) {
    NsFlipper* f = ctx;
    return f->log_open && storage_file_sync(f->log) ? NS_OK : NS_IO;
}
static NsResult log_close(void* ctx) {
    NsFlipper* f = ctx;
    if(!f->log_open) return NS_OK;
    bool ok = storage_file_close(f->log); f->log_open = false;
    return ok ? NS_OK : NS_IO;
}
bool ns_flipper_init(NsFlipper* f) {
    memset(f, 0, sizeof(*f));
    f->storage = furi_record_open(RECORD_STORAGE);
    if(!f->storage) return false;
    f->log = storage_file_alloc(f->storage);
    if(!f->log) { furi_record_close(RECORD_STORAGE); f->storage = NULL; return false; }
    f->platform = (NsPlatform){.ctx = f, .acquire = unavailable, .release = no_release,
        .file_create = log_create, .file_write = log_write, .file_sync = log_sync, .file_close = log_close};
    f->store = (NsStore){f, slot_read, slot_write};
#ifdef NS_ENABLE_BOARD_PORT
    if(!ns_flipper_board_bind(f)) { ns_flipper_deinit(f); return false; }
#endif
    return true;
}
void ns_flipper_deinit(NsFlipper* f) {
#ifdef NS_ENABLE_BOARD_PORT
    ns_flipper_board_unbind(f);
#endif
    log_close(f);
    if(f->log) storage_file_free(f->log);
    if(f->storage) furi_record_close(RECORD_STORAGE);
    f->log = NULL; f->storage = NULL;
}
