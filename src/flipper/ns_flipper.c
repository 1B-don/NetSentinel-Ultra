#include "ns_flipper.h"
#include <furi.h>
#include <string.h>
#define NS_DIRECTORY "/ext/apps_data/netsentinel"
static const char* const slots[] = {NS_DIRECTORY "/state_0.bin", NS_DIRECTORY "/state_1.bin"};
bool ns_flipper_board_valid(const NsBoardPort* b) {
    if(!b) return false;
    const NsPlatform* p = &b->io;
    if((b->nrf || b->cc) && (!p->acquire || !p->release)) return false;
    if(b->nrf && (!p->spi || !p->ce || !p->delay_us)) return false;
    if(b->cc && (!p->cc_probe || !p->cc_receive || !p->cc_rssi || !p->cc_idle)) return false;
    return true;
}
static NsResult ns_flipper_radio_acquire(void* ctx, NsRadio radio) {
    NsFlipper* f = ctx;
    if(f->owner != NS_RADIO_NONE || f->serial) return NS_BUSY;
    NsResult r = NS_UNSUPPORTED;
    if(radio == NS_RADIO_ESP) r = ns_flipper_uart_acquire(f, radio);
    else if((radio == NS_RADIO_NRF && f->capabilities.nrf) || (radio == NS_RADIO_CC && f->capabilities.cc))
        r = f->board.io.acquire(f->board.io.ctx, radio);
    if(r == NS_OK) f->owner = radio;
    return r;
}
void ns_flipper_radio_release(void* ctx, NsRadio radio) {
    NsFlipper* f = ctx;
    if(radio == NS_RADIO_NONE || f->owner != radio) return;
    if(radio == NS_RADIO_ESP) ns_flipper_uart_release(f, radio);
    else {
        if(radio == NS_RADIO_NRF) f->board.io.ce(f->board.io.ctx, false);
        f->board.io.release(f->board.io.ctx, radio);
    }
    f->owner = NS_RADIO_NONE;
}
/* Restrict the board boundary to the register operations used by our RX
 * scanner. Never forward payload, TX/carrier, auto-ACK or retry commands. */
static bool nrf_receive_transfer(const uint8_t* tx, size_t n) {
    if(!tx || n != 2) return false;
    switch(tx[0]) {
    case 0x00: case 0x01: case 0x02: case 0x04: case 0x05: case 0x06: case 0x09:
        return true;
    case 0x20: return tx[1] == 0x01 || tx[1] == 0x03; /* PRIM_RX, power down/up */
    case 0x21: case 0x22: case 0x24: case 0x26: return tx[1] == 0;
    case 0x25: return tx[1] < NS_NRF_CHANNELS;
    default: return false;
    }
}
static NsResult board_spi(void* ctx, const uint8_t* tx, uint8_t* rx, size_t n) {
    NsFlipper* f = ctx;
    if(f->owner != NS_RADIO_NRF || !rx || !tx || n != 2) return NS_INVALID;
    if(!nrf_receive_transfer(tx, n)) return NS_UNSUPPORTED;
    return f->board.io.spi(f->board.io.ctx, tx, rx, n);
}
static void board_ce(void* ctx, bool high) {
    NsFlipper* f = ctx; if(f->owner == NS_RADIO_NRF) f->board.io.ce(f->board.io.ctx, high);
}
static void board_delay(void* ctx, uint32_t us) {
    NsFlipper* f = ctx; if(f->owner == NS_RADIO_NRF) f->board.io.delay_us(f->board.io.ctx, us);
}
static NsResult board_probe(void* ctx) {
    NsFlipper* f = ctx; return f->owner == NS_RADIO_CC ? f->board.io.cc_probe(f->board.io.ctx) : NS_INVALID;
}
static NsResult board_receive(void* ctx, uint32_t hz) {
    NsFlipper* f = ctx; return f->owner == NS_RADIO_CC ? f->board.io.cc_receive(f->board.io.ctx, hz) : NS_INVALID;
}
static NsResult board_rssi(void* ctx, int16_t* out) {
    NsFlipper* f = ctx; return f->owner == NS_RADIO_CC ? f->board.io.cc_rssi(f->board.io.ctx, out) : NS_INVALID;
}
static NsResult board_idle(void* ctx) {
    NsFlipper* f = ctx; return f->owner == NS_RADIO_CC ? f->board.io.cc_idle(f->board.io.ctx) : NS_INVALID;
}
#ifdef NS_ENABLE_BOARD_PORT
static NsResult board_diagnose(void* ctx, NsCcDiagnostics* out) {
    NsFlipper* f = ctx; return f->owner == NS_RADIO_CC ? f->board.io.cc_diagnose(f->board.io.ctx, out) : NS_INVALID;
}
#endif
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
    atomic_init(&f->rx_head, 0); atomic_init(&f->rx_tail, 0); atomic_init(&f->rx_error, NS_OK);
    f->capabilities.esp_uart = true; /* command-capture gate remains false */
    f->storage = furi_record_open(RECORD_STORAGE);
    if(!f->storage) return false;
    f->log = storage_file_alloc(f->storage);
    if(!f->log) { furi_record_close(RECORD_STORAGE); f->storage = NULL; return false; }
    f->platform = (NsPlatform){.ctx = f, .acquire = ns_flipper_radio_acquire, .release = ns_flipper_radio_release,
        .spi = board_spi, .ce = board_ce, .delay_us = board_delay,
        .cc_probe = board_probe, .cc_receive = board_receive, .cc_rssi = board_rssi, .cc_idle = board_idle,
        .uart_write = ns_flipper_uart_write, .uart_read = ns_flipper_uart_read,
        .file_create = log_create, .file_write = log_write, .file_sync = log_sync, .file_close = log_close};
    f->store = (NsStore){f, slot_read, slot_write};
#ifdef NS_ENABLE_BOARD_PORT
    if(!ns_flipper_board_bind(&f->board) || !ns_flipper_board_valid(&f->board)) { ns_flipper_deinit(f); return false; }
    f->capabilities.nrf = f->board.nrf; f->capabilities.cc = f->board.cc;
    if(f->board.io.cc_diagnose) f->platform.cc_diagnose = board_diagnose;
#endif
    return true;
}
void ns_flipper_deinit(NsFlipper* f) {
    ns_flipper_radio_release(f, f->owner);
#ifdef NS_ENABLE_BOARD_PORT
    ns_flipper_board_unbind(&f->board);
#endif
    ns_flipper_uart_release(f, NS_RADIO_ESP);
    log_close(f);
    if(f->log) storage_file_free(f->log);
    if(f->storage) furi_record_close(RECORD_STORAGE);
    f->log = NULL; f->storage = NULL;
}
