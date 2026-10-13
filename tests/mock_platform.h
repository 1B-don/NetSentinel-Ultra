#ifndef NS_MOCK_PLATFORM_H
#define NS_MOCK_PLATFORM_H
#include "netsentinel/session.h"
#include <assert.h>
#include <string.h>
typedef struct {
    NsRadio owner;
    uint8_t regs[32];
    bool ce, fail_spi, absent, fail_write, fail_sync, fail_uart, fail_idle, fail_close;
    unsigned acquired, released, transfers, delay, ce_high, closes, writes, uart_calls, fail_at;
    int16_t rssi;
    uint32_t frequency;
    char file[8192]; size_t size;
} Mock;
static NsResult acquire(void* ctx, NsRadio r) {
    Mock* m = ctx; if(m->owner) return NS_BUSY;
    m->owner = r; m->acquired++; return NS_OK;
}
static void release(void* ctx, NsRadio r) {
    Mock* m = ctx; assert(m->owner == r); assert(!m->ce);
    m->owner = NS_RADIO_NONE; m->released++;
}
static NsResult spi(void* ctx, const uint8_t* tx, uint8_t* rx, size_t n) {
    Mock* m = ctx; assert(m->owner == NS_RADIO_NRF); assert(n == 2);
    assert(tx[0] < 0x40); /* no TX payload/commands */
    m->transfers++; if(m->fail_spi || m->transfers == m->fail_at) return NS_IO;
    unsigned reg = tx[0] & 31;
    if(tx[0] & 0x20) {
        if(reg == 0) assert(tx[1] & 1); /* always PRX */
        if(reg == 1 || reg == 4) assert(tx[1] == 0);
        if(reg == 6) assert(!(tx[1] & 0x90)); /* no continuous carrier */
        m->regs[reg] = tx[1];
    }
    rx[0] = 0x0e; rx[1] = m->absent ? 0xff : m->regs[reg]; return NS_OK;
}
static void ce(void* ctx, bool high) {
    Mock* m = ctx;
    if(high) { assert(m->regs[0] == 3 && !m->regs[1] && !m->regs[4]); m->ce_high++; }
    m->ce = high;
}
static void delay(void* ctx, uint32_t us) { ((Mock*)ctx)->delay += us; }
static NsResult uart(void* ctx, const uint8_t* data, size_t n) {
    Mock* m = ctx; assert(data && n); m->uart_calls++;
    return m->fail_uart ? NS_IO : NS_OK;
}
static NsResult probe(void* ctx) { return ((Mock*)ctx)->absent ? NS_ABSENT : NS_OK; }
static NsResult receive(void* ctx, uint32_t f) { ((Mock*)ctx)->frequency = f; return NS_OK; }
static NsResult rssi(void* ctx, int16_t* v) { *v = ((Mock*)ctx)->rssi; return NS_OK; }
static NsResult idle(void* ctx) { Mock* m = ctx; m->frequency = 0; return m->fail_idle ? NS_IO : NS_OK; }
static NsResult create(void* ctx, const char* path) {
    Mock* m = ctx; assert(path); m->size = 0; m->file[0] = 0; return NS_OK;
}
static NsResult write_file(void* ctx, const char* text, size_t n) {
    Mock* m = ctx; m->writes++;
    if(m->fail_write) return NS_IO;
    if(m->size + n >= sizeof(m->file)) return NS_FULL;
    memcpy(m->file + m->size, text, n); m->size += n; m->file[m->size] = 0; return NS_OK;
}
static NsResult sync_file(void* ctx) { return ((Mock*)ctx)->fail_sync ? NS_IO : NS_OK; }
static NsResult close_file(void* ctx) { Mock* m = ctx; m->closes++; return m->fail_close ? NS_IO : NS_OK; }
static NsPlatform platform(Mock* m) {
    return (NsPlatform){m, acquire, release, spi, ce, delay, uart, probe, receive, rssi, idle,
        create, write_file, sync_file, close_file};
}
#endif
