#!/bin/bash
# Move TelemetryPacket and getTelemetryBinary into the AppLogic namespace to resolve 'not declared in this scope' errors when referenced by outer sketches.
# Also fix deprecated ArduinoJson usages as indicated in memory.

cat << 'INNEREOF' > libraries/LD2420_Ultimate/src/LD2420_AppLogic.hpp.patch
<<<<<<< SEARCH
#pragma pack(push, 1)
struct TelemetryPacket {
    uint16_t magic; // 0xBEEF
    uint8_t presence;
    uint8_t activity;
    int16_t range_cm;
    int16_t velocity_cm_s;
    float accel_cm_s2;
    float jerk_cm_s3;
    float cadence_hz;
    float breathing_bpm;
    float heart_rate_bpm;
    uint8_t posture_class;
    uint8_t sleep_stage;
    uint8_t anomaly_score; // 0-255 mapped from 0.0-1.0
    uint8_t intent_leaving;
    uint8_t voice_prime;
    uint8_t zones[8]; // 0-255 mapped from 0.0-1.0
    uint32_t uptime_s;
    uint16_t checksum;
};
#pragma pack(pop)

inline void getTelemetryBinary(TelemetryPacket& pkt) {
    pkt.magic = 0xBEEF;
    pkt.presence = radar.presence_fused ? 1 : 0;
    pkt.activity = (uint8_t)radar.activity;
    pkt.range_cm = radar.range_cm;
    pkt.velocity_cm_s = radar.velocity_cm_s;
    pkt.accel_cm_s2 = radar.accel_cm_s2;
    pkt.jerk_cm_s3 = radar.jerk_cm_s3;
    pkt.cadence_hz = radar.cadence_hz;
    pkt.breathing_bpm = radar.breathing_rate_bpm;
    pkt.heart_rate_bpm = radar.heart_rate_bpm;
    pkt.posture_class = radar.posture_class;
    pkt.sleep_stage = radar.sleep_stage;
    pkt.anomaly_score = (uint8_t)(radar.anomaly_score * 255.0);
    pkt.intent_leaving = radar.intent_leaving ? 1 : 0;
    pkt.voice_prime = radar.voice_prime ? 1 : 0;
    for(int i=0; i<8; i++) pkt.zones[i] = (uint8_t)(radar.zone_prob[i] * 255.0);
    pkt.uptime_s = radar.uptime_s;

    // simple checksum
    pkt.checksum = 0;
    uint8_t* ptr = (uint8_t*)&pkt;
    for(size_t i=0; i<sizeof(TelemetryPacket)-2; i++) {
        pkt.checksum += ptr[i];
    }
}
=======
>>>>>>> REPLACE
INNEREOF

# Note: We'll do this patch manually step by step with the sed or Python tool to be more precise
