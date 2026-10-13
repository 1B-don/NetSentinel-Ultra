#include "../../include/netsentinel/application.h"
#include <stdio.h>
#include <string.h>
NsResult ns_app_export_history(NsApplication* a, uint32_t now) {
    if(!a) return NS_INVALID;
    if(a->session.active != NS_RADIO_NONE || a->info_pending || a->history_dirty) return a->status = NS_BUSY;
    const NsPlatform* io = a->session.io;
    if(!io || !io->file_create || !io->file_write || !io->file_sync || !io->file_close) return a->status = NS_UNSUPPORTED;
    if(a->saved.last_id == UINT32_MAX) return a->status = NS_FULL;
    NsSavedState next = a->saved; next.last_id++;
    NsResult r = ns_persistence_save(&a->persistence, &next);
    a->storage_status = r;
    if(r != NS_OK) return a->status = r;
    a->saved = next; a->session.id = next.last_id; /* reserve before exclusive create */
    char path[100];
    snprintf(path, sizeof(path), "/ext/apps_data/netsentinel/session_%010lu.csv", (unsigned long)next.last_id);
    r = io->file_create(io->ctx, path);
    if(r != NS_OK) return a->status = r;
    const char* header = "session,time_ms,kind,label,channel_or_hz,value,unit\n";
    r = io->file_write(io->ctx, header, strlen(header));
    for(size_t i = 0; r == NS_OK; i++) {
        const NsHistory* h = ns_history_select(&a->saved, &a->history_filter, i);
        if(!h) break;
        const char* labels[] = {"duration", "observations", "result"};
        uint32_t values[] = {h->ended_ms-h->started_ms, h->observations, (uint32_t)h->result};
        for(unsigned field = 0; field < 3 && r == NS_OK; field++) {
            char row[128];
            int n = snprintf(row, sizeof(row), "%lu,%lu,history_%s,radio_%u,%lu,%lu,%s\n",
                (unsigned long)next.last_id, (unsigned long)now, labels[field], (unsigned)h->radio,
                (unsigned long)h->id, (unsigned long)values[field], field == 0 ? "ms" : (field == 2 ? "enum" : "count"));
            r = n < 0 || (size_t)n >= sizeof(row) ? NS_FULL : io->file_write(io->ctx, row, (size_t)n);
        }
    }
    if(r == NS_OK) r = io->file_sync(io->ctx);
    NsResult closed = io->file_close(io->ctx); /* release even after short write/sync failure */
    if(r == NS_OK) r = closed;
    if(r == NS_OK) a->export_id = next.last_id;
    return a->status = r;
}
