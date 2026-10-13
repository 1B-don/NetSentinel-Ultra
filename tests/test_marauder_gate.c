#include "netsentinel/marauder.h"
#include <assert.h>
#include <stdio.h>

typedef struct { unsigned writes; } Mock;
static NsResult uart_write(void* ctx, const uint8_t* bytes, size_t count) {
    Mock* mock = ctx;
    (void)bytes;
    (void)count;
    mock->writes++;
    return NS_OK;
}

int main(void) {
    Mock mock = {0};
    NsPlatform io = {.ctx = &mock, .uart_write = uart_write};
    NsMarauder marauder;

    /* Matching identity alone must not unlock a command whose common
     * initializer can start an AP and whose capture can write PCAP. */
    assert(ns_marauder_begin(&marauder, &io, true, true, 5000, 0) == NS_UNSUPPORTED);
    assert(mock.writes == 0);

    /* The approved information-only transaction remains available. */
    assert(ns_marauder_begin(&marauder, &io, false, false, 5000, 1) == NS_OK);
    assert(mock.writes == 1);
    puts("PASS: Marauder passive-start safety gate and info-only command (default build)");
    return 0;
}
