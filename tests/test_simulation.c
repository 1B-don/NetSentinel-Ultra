#include "netsentinel/simulation.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    NsSimulation a, b; ns_simulation_init(&a); ns_simulation_init(&b);
    assert(ns_simulation_generate(&a, 42, SIZE_MAX) == NS_SIM_BATCH_LIMIT);
    assert(ns_simulation_generate(&b, 42, NS_SIM_BATCH_LIMIT) == NS_SIM_BATCH_LIMIT);
    assert(!memcmp(a.frames, b.frames, NS_SIM_BATCH_LIMIT*sizeof(a.frames[0])));
    for(size_t i=0;i<a.count;i++) {
        const NsSyntheticFrame* f=&a.frames[i];
        assert(f->protocol>=NS_SIM_WIFI_EVENT&&f->protocol<=NS_SIM_SUBGHZ_EVENT);
        assert(f->channel>=1&&f->channel<=NS_SIM_CHANNEL_LIMIT);
        assert(f->payload_length>=1&&f->payload_length<=NS_SIM_PAYLOAD_MAX);
        assert(f->signal_dbm>=-109&&f->signal_dbm<=-35);
        assert(f->frame_size<=NS_SIM_FRAME_BYTES&&ns_simulation_frame_valid(f));
    }
    assert(!ns_simulation_replay_step(&a, 0, NULL));
    NsSyntheticFrame corrupt=a.frames[0]; corrupt.frame_bytes[4]=2;
    assert(!ns_simulation_frame_valid(&corrupt));
    corrupt=a.frames[0]; corrupt.frame_bytes[12]^=0x80;
    assert(!ns_simulation_frame_valid(&corrupt));
    assert(ns_simulation_replay_begin(&a, UINT32_MAX-5));
    NsSyntheticFrame frame;
    for(uint32_t i=0;i<NS_SIM_BATCH_LIMIT;i++)
        assert(ns_simulation_replay_step(&a, UINT32_MAX-5+i, &frame) && frame.sequence==i+1);
    assert(!a.replay_active&&a.replayed==NS_SIM_BATCH_LIMIT&&a.replay_elapsed_ms==15);
    assert(ns_simulation_replay_begin(&a, 100));
    assert(ns_simulation_generate(&a, 1, 1)==0);
    ns_simulation_replay_cancel(&a);
    assert(ns_simulation_generate(&a, 1, 0)==0);
    uint8_t loads[NS_SIM_CHANNEL_LIMIT]={0};
    assert(ns_simulation_model(&a,loads)&&a.model_quality==100&&a.model_interference==0&&!a.model_high_overlap);
    memset(loads,100,sizeof(loads));
    assert(ns_simulation_model(&a,loads)&&a.model_quality==0&&a.model_interference==100&&a.model_high_overlap);
    loads[4]=101;
    assert(!ns_simulation_model(&a,loads)&&a.model_interference==100&&!a.model_valid&&!a.model_high_overlap);
    assert(ns_simulation_generate(&a, 5, 100)==NS_SIM_BATCH_LIMIT);
    assert(ns_simulation_generate(&a, 5, 100)==NS_SIM_BATCH_LIMIT);
    assert(ns_simulation_generate(&a, 5, 100)==NS_SIM_BATCH_LIMIT);
    assert(a.count==NS_SIM_FRAME_LIMIT);
    assert(ns_simulation_generate(&a, 5, 1)==0);
    ns_simulation_init(&a);
    assert(!a.count&&!a.model_valid&&!a.replay_active);
    puts("PASS: deterministic bounded synthetic generation, local replay, cancellation and offline occupancy model");
}
