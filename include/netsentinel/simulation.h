#ifndef NS_SIMULATION_H
#define NS_SIMULATION_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Offline-only NSIM frames. The private format is not an 802.11, BLE, NRF24 or
 * Sub-GHz wire format and has no hardware callback or radio serializer.
 */
#define NS_SIM_FRAME_LIMIT 64u
#define NS_SIM_BATCH_LIMIT 16u
#define NS_SIM_CHANNEL_LIMIT 14u
#define NS_SIM_REPLAY_WINDOW_MS 10u
#define NS_SIM_FRAME_BYTES 48u
#define NS_SIM_PAYLOAD_MAX 32u
typedef enum { NS_SIM_WIFI_EVENT = 1, NS_SIM_BLE_EVENT = 2, NS_SIM_NRF_EVENT = 3, NS_SIM_SUBGHZ_EVENT = 4 } NsSimProtocol;
typedef struct {
    uint32_t sequence;
    uint32_t at_ms;
    uint32_t payload_fingerprint;
    uint16_t payload_length;
    uint8_t protocol;
    uint8_t channel;
    int8_t signal_dbm;
    uint8_t frame_size;
    uint8_t frame_bytes[NS_SIM_FRAME_BYTES];
} NsSyntheticFrame;
typedef struct {
    NsSyntheticFrame frames[NS_SIM_FRAME_LIMIT];
    size_t count;
    size_t replay_position;
    uint32_t replayed;
    uint32_t replay_elapsed_ms;
    uint32_t replay_started_ms;
    uint32_t generator_state;
    uint8_t channel_load[NS_SIM_CHANNEL_LIMIT];
    uint8_t model_quality;
    uint8_t model_interference;
    bool replay_active;
    bool model_valid;
    bool model_high_overlap;
} NsSimulation;

void ns_simulation_init(NsSimulation* simulation);
size_t ns_simulation_generate(NsSimulation* simulation, uint32_t seed, size_t requested);
bool ns_simulation_frame_valid(const NsSyntheticFrame* frame);
bool ns_simulation_replay_begin(NsSimulation* simulation, uint32_t now_ms);
bool ns_simulation_replay_step(NsSimulation* simulation, uint32_t now_ms, NsSyntheticFrame* frame);
void ns_simulation_replay_cancel(NsSimulation* simulation);
/* Loads are synthetic 0..100 values; score is a deterministic educational estimate, not RF data. */
bool ns_simulation_model(NsSimulation* simulation, const uint8_t channel_load[NS_SIM_CHANNEL_LIMIT]);
#endif
