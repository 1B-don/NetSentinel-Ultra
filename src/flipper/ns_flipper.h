#ifndef NS_FLIPPER_H
#define NS_FLIPPER_H
#include "../../include/netsentinel/application.h"
#include <storage/storage.h>
typedef struct {
    Storage* storage;
    File* log;
    bool log_open;
    void* board_context;
    NsPlatform platform;
    NsStore store;
    NsCapabilities capabilities;
} NsFlipper;
bool ns_flipper_init(NsFlipper*);
void ns_flipper_deinit(NsFlipper*);
/* Optional, independently authorized hardware port. bind may populate ONLY
 * radio callbacks/capabilities and board_context. It must not touch power/pins
 * until acquire, and must leave ESP unsupported. unbind frees port resources. */
#ifdef NS_ENABLE_BOARD_PORT
bool ns_flipper_board_bind(NsFlipper*);
void ns_flipper_board_unbind(NsFlipper*);
#endif
#endif
