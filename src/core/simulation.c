#include "../../include/netsentinel/simulation.h"
#include <string.h>

static uint32_t next_value(NsSimulation* s) {
    s->generator_state = s->generator_state * 1664525u + 1013904223u;
    return s->generator_state;
}

static uint16_t crc16(const uint8_t* bytes, size_t length) {
    uint16_t crc = 0xffffu;
    for(size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)bytes[i] << 8;
        for(unsigned bit = 0; bit < 8; bit++)
            crc = (uint16_t)((crc << 1) ^ ((crc & 0x8000u) ? 0x1021u : 0u));
    }
    return crc;
}

void ns_simulation_init(NsSimulation* s) {
    if(s) memset(s, 0, sizeof(*s));
}

size_t ns_simulation_generate(NsSimulation* s, uint32_t seed, size_t requested) {
    if(!s || s->replay_active) return 0;
    if(requested > NS_SIM_BATCH_LIMIT) requested = NS_SIM_BATCH_LIMIT;
    if(requested > NS_SIM_FRAME_LIMIT - s->count) requested = NS_SIM_FRAME_LIMIT - s->count;
    s->model_valid = false;
    if(!seed) seed = 0x4e53554cu;
    s->generator_state = seed;
    for(size_t i = 0; i < requested; i++) {
        uint32_t a = next_value(s), b = next_value(s);
        NsSyntheticFrame* f = &s->frames[s->count++];
        f->sequence = (uint32_t)s->count;
        f->at_ms = (uint32_t)i * 25u;
        f->payload_length = (uint16_t)(1u + (b % NS_SIM_PAYLOAD_MAX));
        f->protocol = (uint8_t)(NS_SIM_WIFI_EVENT + (a % 4u));
        /* A neutral laboratory bin only. This is not a hardware channel map. */
        f->channel = (uint8_t)(1u + ((b >> 8) % NS_SIM_CHANNEL_LIMIT));
        f->signal_dbm = (int8_t)(-35 - (int)(a % 75u));
        memcpy(f->frame_bytes, "NSIM", 4);
        f->frame_bytes[4] = 1;
        f->frame_bytes[5] = f->protocol;
        f->frame_bytes[6] = f->channel;
        f->frame_bytes[7] = (uint8_t)f->payload_length;
        f->frame_bytes[8] = (uint8_t)f->sequence;
        f->frame_bytes[9] = (uint8_t)(f->sequence >> 8);
        f->frame_bytes[10] = (uint8_t)(f->sequence >> 16);
        f->frame_bytes[11] = (uint8_t)(f->sequence >> 24);
        uint32_t fingerprint = 2166136261u;
        for(size_t byte = 0; byte < f->payload_length; byte++) {
            uint8_t value = (uint8_t)next_value(s);
            f->frame_bytes[12+byte] = value;
            fingerprint = (fingerprint ^ value) * 16777619u;
        }
        f->payload_fingerprint = fingerprint;
        size_t crc_pos = 12u + f->payload_length;
        uint16_t checksum = crc16(f->frame_bytes, crc_pos);
        f->frame_bytes[crc_pos] = (uint8_t)checksum;
        f->frame_bytes[crc_pos+1] = (uint8_t)(checksum >> 8);
        f->frame_size = (uint8_t)(crc_pos+2u);
    }
    return requested;
}

bool ns_simulation_frame_valid(const NsSyntheticFrame* f) {
    if(!f || f->frame_size < 15u || f->frame_size > NS_SIM_FRAME_BYTES ||
       f->frame_bytes[0] != 'N' || f->frame_bytes[1] != 'S' ||
       f->frame_bytes[2] != 'I' || f->frame_bytes[3] != 'M' ||
       f->frame_bytes[4] != 1 || f->frame_bytes[5] < NS_SIM_WIFI_EVENT ||
       f->frame_bytes[5] > NS_SIM_SUBGHZ_EVENT || f->frame_bytes[6] < 1 ||
       f->frame_bytes[6] > NS_SIM_CHANNEL_LIMIT || f->frame_bytes[7] < 1 ||
       f->frame_bytes[7] > NS_SIM_PAYLOAD_MAX ||
       f->frame_size != (uint8_t)(14u + f->frame_bytes[7])) return false;
    uint32_t sequence = (uint32_t)f->frame_bytes[8] | ((uint32_t)f->frame_bytes[9] << 8) |
        ((uint32_t)f->frame_bytes[10] << 16) | ((uint32_t)f->frame_bytes[11] << 24);
    size_t crc_pos = 12u + f->frame_bytes[7];
    uint16_t stored = (uint16_t)f->frame_bytes[crc_pos] | ((uint16_t)f->frame_bytes[crc_pos+1] << 8);
    return f->frame_bytes[5] == f->protocol && f->frame_bytes[6] == f->channel &&
        f->frame_bytes[7] == f->payload_length && sequence == f->sequence &&
        crc16(f->frame_bytes, crc_pos) == stored;
}

bool ns_simulation_replay_begin(NsSimulation* s, uint32_t now_ms) {
    if(!s || !s->count || s->replay_active) return false;
    s->replay_position = 0;
    s->replayed = 0;
    s->replay_elapsed_ms = 0;
    s->replay_started_ms = now_ms;
    s->replay_active = true;
    return true;
}

bool ns_simulation_replay_step(NsSimulation* s, uint32_t now_ms, NsSyntheticFrame* out) {
    if(!s || !s->replay_active || !out || s->replay_position >= s->count) return false;
    *out = s->frames[s->replay_position++];
    s->replayed++;
    s->replay_elapsed_ms = now_ms - s->replay_started_ms;
    if(s->replay_position == s->count) s->replay_active = false;
    return true;
}

void ns_simulation_replay_cancel(NsSimulation* s) {
    if(s) s->replay_active = false;
}

bool ns_simulation_model(NsSimulation* s, const uint8_t load[NS_SIM_CHANNEL_LIMIT]) {
    if(!s || !load) return false;
    s->model_valid = false;
    s->model_high_overlap = false;
    for(size_t i = 0; i < NS_SIM_CHANNEL_LIMIT; i++) if(load[i] > 100) return false;
    uint32_t total = 0, adjacent = 0;
    for(size_t i = 0; i < NS_SIM_CHANNEL_LIMIT; i++) {
        s->channel_load[i] = load[i];
        total += load[i];
        uint8_t left = load[i ? i-1 : i];
        uint8_t right = load[i+1 < NS_SIM_CHANNEL_LIMIT ? i+1 : i];
        adjacent += ((uint32_t)left + right) / 2u;
    }
    uint32_t own_mean = total / NS_SIM_CHANNEL_LIMIT;
    uint32_t adjacent_mean = adjacent / NS_SIM_CHANNEL_LIMIT;
    uint32_t interference = (own_mean * 3u + adjacent_mean) / 4u;
    s->model_interference = (uint8_t)(interference > 100 ? 100 : interference);
    s->model_quality = (uint8_t)(100u - s->model_interference);
    s->model_high_overlap = s->model_interference >= 65;
    s->model_valid = true;
    return true;
}
