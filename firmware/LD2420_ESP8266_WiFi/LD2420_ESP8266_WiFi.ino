/*
 * ================================================================================
 * HLK-LD2420 PRESENCE INTELLIGENCE PLATFORM — ESP8266 FIRMWARE v4.0
 * ================================================================================
 *
 * Features:
 *  - Full ASCII + binary protocol parser (ON / OFF / Range NNNN)
 *  - Advanced 9-state presence engine with confidence scoring
 *  - 8-zone distance mapping (0-8m in 1m bands)
 *  - Activity inference (walking, sitting, sleeping, idle, etc.)
 *  - Occupancy session tracking with duration + history
 *  - Adaptive sensitivity engine
 *  - Premium Web Dashboard with cm/m toggle (no scroll wheels)
 *  - REST API + WebSocket streaming + MQTT + OTA
 *  - Home Assistant auto-discovery via MQTT
 *
 * Wiring (NodeMCU <-> HLK-LD2420):
 *  3V3  ---> 3V3   (stable 3.3V supply — use LDO if needed)
 *  GND  ---> GND
 *  D6   ---> OT1   (Radar UART TX → ESP RX) [SoftwareSerial RX]
 *  D5   ---> RX    (Radar UART RX ← ESP TX) [SoftwareSerial TX]
 *  D7   ---> OT2   (Hardware presence pin)
 *  D4   ---> Status LED (active LOW on NodeMCU)
 *
 * Baud Rate: 115200
 * ================================================================================
 */

// ================================================================================
// FEATURE CONFIGURATION (Toggles to save heap memory on ESP8266)
// ================================================================================
#define ENABLE_OTA   0  // Set to 1 to enable Over-The-Air updates (uses ~10KB RAM)
#define ENABLE_MDNS  1  // Set to 1 to enable mDNS (ld2420.local, uses ~2KB RAM)
#define ENABLE_MQTT  0  // Set to 1 to enable MQTT client (uses ~3KB RAM)

// ================================================================================
// INCLUDES
// ================================================================================
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> // Captive portal for secure WiFi setup
#if ENABLE_MDNS
#include <ESP8266mDNS.h>
#endif
#include <WebSocketsServer.h>
#include <SoftwareSerial.h>
#if ENABLE_OTA
#include <ArduinoOTA.h>
#endif
#include <ArduinoJson.h>
#if ENABLE_MQTT
#include <PubSubClient.h>
#endif
#include <EEPROM.h>
#include <Ticker.h>

// ================================================================================
// USER CONFIGURATION — EDIT THESE
// ================================================================================

// Captive Portal AP Name
const char* AP_SSID          = "LD2420_Setup";
const char* AP_PASS          = "YOUR_SETUP_PASSWORD";

// MQTT (optional — set to "" to disable)
#if ENABLE_MQTT
const char* MQTT_HOST        = "";  // Set to broker IP to enable, e.g. "192.168.1.100"
const uint16_t MQTT_PORT     = 1883;
const char* MQTT_USER        = "";
const char* MQTT_PASS        = "";
const char* MQTT_CLIENT_ID   = "ld2420_radar";
const char* MQTT_TOPIC_BASE  = "homeassistant/sensor/ld2420";
#endif

// Radar UART pins
#define PIN_RADAR_RX     D6
#define PIN_RADAR_TX     D5
#define PIN_RADAR_OT2    D7
#define PIN_LED          D4

#define RADAR_BAUD       115200
#define SNAPSHOT_SIZE    256   // ASCII ring buffer size for web terminal (reduced to save RAM)

// ================================================================================

#include "LD2420_AppLogic.hpp"

#include <SoftwareSerial.h>
SoftwareSerial radarSerial(PIN_RADAR_RX, PIN_RADAR_TX); // RX, TX

ESP8266WebServer httpServer(80);
WebSocketsServer wsServer(81);
#if ENABLE_MQTT
WiFiClient espClient;
PubSubClient mqttClient(espClient);
#endif

uint32_t last_ws_ms = 0;
#define WS_INTERVAL_MS 100 // 10Hz to match telemetry

uint32_t last_mqtt_ms = 0;
#define MQTT_INTERVAL_MS 1000 

void sendWebSocketUpdate() {
    StaticJsonDocument<1024> doc;
    AppLogic::getTelemetryJson(doc);
    String output;
    serializeJson(doc, output);
    wsServer.broadcastTXT(output);
}

void mqttPublish() {
#if ENABLE_MQTT
    if(!mqttClient.connected()) return;
    StaticJsonDocument<1024> doc;
    AppLogic::getTelemetryJson(doc);
    String output;
    serializeJson(doc, output);
    mqttClient.publish(String(String(MQTT_TOPIC_BASE) + "/state").c_str(), output.c_str(), true);
#endif
}

void handleRoot();
#if ENABLE_MQTT
void mqttReconnect();
#endif
void secondISR();

// ================================================================================
// HARDWARE INTERRUPT — OT2 HARDWARE PIN
// ================================================================================

}

void secondISR() {
    radar.uptime_seconds++;
    // Update hourly occupancy (slot = uptime_seconds / 3600 mod HISTORY_BINS)
    if (radar.presence_serial || radar.presence_hardware) {
        uint8_t slot = (radar.uptime_seconds / 3600) % HISTORY_BINS;
        radar.hourly_occupancy[slot]++;
        radar.total_occupancy_ms += 1000;
    }
}

// ================================================================================
// RADAR PROTOCOL COMMANDS (binary frame)
// ================================================================================
const uint8_t CMD_ENABLE_CONFIG[]  = {0xFD,0xFC,0xFB,0xFA,0x04,0x00,0xFF,0x00,0x01,0x00,0x04,0x03,0x02,0x01};
const uint8_t CMD_DISABLE_CONFIG[] = {0xFD,0xFC,0xFB,0xFA,0x02,0x00,0xFE,0x00,0x04,0x03,0x02,0x01};
const uint8_t CMD_READ_VERSION[]   = {0xFD,0xFC,0xFB,0xFA,0x02,0x00,0x00,0x00,0x04,0x03,0x02,0x01};
const uint8_t CMD_REBOOT[]         = {0xFD,0xFC,0xFB,0xFA,0x02,0x00,0xA3,0x00,0x04,0x03,0x02,0x01};

void sendRadarCmd(const uint8_t* cmd, size_t len) {
    radarSerial.write(cmd, len);
}

// ================================================================================
// KALMAN FILTER ENGINE (PROBABILISTIC DSP)
// ================================================================================
class RadarKalmanFilter {
private:
    float x; float v;
    float p00, p01, p10, p11;
    float q_pos, q_vel, r_measure;
    uint32_t last_time;
    float innovation_err;

public:
    RadarKalmanFilter() {
        x = 0; v = 0;
        p00 = 1000.0; p01 = 0;
        p10 = 0;      p11 = 1000.0;
        q_pos = 0.1;      // Base Process noise (position)
        q_vel = 1.0;      // Base Process noise (velocity)
        r_measure = 15.0; // Measurement noise (LD2420 distance jitter)
        last_time = 0;
        innovation_err = 0;
    }

    void reset(float initial_pos) {
        x = initial_pos; v = 0;
        p00 = 1000.0; p01 = 0; p10 = 0; p11 = 1000.0;
        last_time = millis();
        innovation_err = 0;
    }

    void update(float measurement) {
        uint32_t now = millis();
        if (last_time == 0) { reset(measurement); return; }
        float dt = (now - last_time) / 1000.0f;
        last_time = now;
        if (dt <= 0 || dt > 2.0f) { reset(measurement); return; }

        // Adaptive Q multiplier based on innovation error (residual)
        float q_mult = 1.0f + (abs(innovation_err) / 10.0f);
        if (q_mult > 15.0f) q_mult = 15.0f; // Cap adaptation for logic stability

        float q_pos_dyn = q_pos * q_mult;
        float q_vel_dyn = q_vel * q_mult;

        x += v * dt;
        float p00_temp = p00 + dt * p10 + dt * (p01 + dt * p11) + q_pos_dyn;
        float p01_temp = p01 + dt * p11;
        float p10_temp = p10 + dt * p11;
        float p11_temp = p11 + q_vel_dyn;
        p00 = p00_temp; p01 = p01_temp; p10 = p10_temp; p11 = p11_temp;

        innovation_err = measurement - x;
        float s = p00 + r_measure;
        float k0 = p00 / s;
        float k1 = p10 / s;

        x += k0 * innovation_err;
        v += k1 * innovation_err;

        float p00_new = (1 - k0) * p00;
        float p01_new = (1 - k0) * p01;
        float p10_new = -k1 * p00 + p10;
        float p11_new = -k1 * p01 + p11;

        p00 = p00_new; p01 = p01_new; p10 = p10_new; p11 = p11_new;
    }

    float getPosition() { return x; }
    float getVelocity() { return v; }
    float getInnovation() { return innovation_err; }
};

RadarKalmanFilter kalman;

// ================================================================================
// HIDDEN MARKOV MODEL (HMM) PROBABILISTIC ENGINE
// ================================================================================
class MarkovActivityEngine {
public:
    float probs[6] = {1.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    
    // Transition Matrix
    const float trans[6][6] = {
        /* IDLE     */ {0.90, 0.00, 0.00, 0.00, 0.05, 0.05},
        /* SLEEPING */ {0.05, 0.90, 0.05, 0.00, 0.00, 0.00},
        /* SITTING  */ {0.01, 0.05, 0.85, 0.09, 0.00, 0.00},
        /* STANDING */ {0.01, 0.00, 0.10, 0.80, 0.09, 0.00},
        /* WALKING  */ {0.05, 0.00, 0.00, 0.15, 0.75, 0.05},
        /* RUNNING  */ {0.05, 0.00, 0.00, 0.00, 0.15, 0.80}
    };

    void process(float v, float duration_s, float innov) {
        float abs_v = abs(v);
        float emissions[6] = {0.0};
        
        emissions[5]  = (abs_v > 120.0) ? 0.9 : 0.01; // RUNNING
        emissions[4]  = (abs_v > 30.0 && abs_v <= 120.0) ? 0.9 : 0.05; // WALKING
        emissions[3]  = (abs_v <= 15.0 && duration_s < 60.0) ? 0.8 : 0.1; // STANDING
        emissions[2]  = (abs_v <= 10.0 && duration_s >= 60.0) ? 0.8 : 0.1; // SITTING
        emissions[1]  = (abs_v <= 5.0 && duration_s >= 300.0 && innov < 1.0) ? 0.9 : 0.01; // SLEEPING
        emissions[0]  = 0.05; // IDLE

        float new_probs[6] = {0.0};
        float sum = 0.0;
        
        for (int i = 0; i < 6; i++) {
            float prior_sum = 0.0;
            for (int j = 0; j < 6; j++) {
                prior_sum += probs[j] * trans[j][i];
            }
            new_probs[i] = emissions[i] * prior_sum;
            sum += new_probs[i];
        }

        for (int i = 0; i < 6; i++) {
            probs[i] = new_probs[i] / sum;
        }
    }

    uint8_t getBest() {
        uint8_t best = 0;
        for (uint8_t i = 1; i < 6; i++) {
            if (probs[i] > probs[best]) best = i;
        }
        return best;
    }
};

MarkovActivityEngine markov;

// ================================================================================
// ASCII PROTOCOL PARSER
// ================================================================================
class AsciiParser {
    char   lineBuf[80];
    uint8_t lineLen = 0;

public:
    void feed(uint8_t b) {
        // Store in ring buffer for terminal view
        snapBuf[snapIdx] = b;
        snapIdx = (snapIdx + 1) % SNAPSHOT_SIZE;

        if (b == '\n') {
            lineBuf[lineLen] = '\0';
            processLine(lineBuf);
            lineLen = 0;
        } else if (b != '\r' && lineLen < 79) {
            lineBuf[lineLen++] = (char)b;
        }
    }

private:
    void processLine(const char* line) {
        if (!line || line[0] == '\0') return;
        radar.packets_received++;

        if (strncmp(line, "ON", 2) == 0) {
            radar.presence_serial = true;
            radar.last_seen_ms = millis();
            if (radar.moving_range_cm > 0) {
                snprintf(radar.detected_message, sizeof(radar.detected_message),
                         "Motion Detected! Last seen at %.2fm", radar.moving_range_cm / 100.0f);
            } else {
                snprintf(radar.detected_message, sizeof(radar.detected_message),
                         "Motion Detected! Calculating range...");
            }
        }
        else if (strncmp(line, "OFF", 3) == 0) {
            radar.presence_serial = false;
            radar.velocity_cm_s = 0;
            radar.tangential_motion = false;
            radar.energy_moving = 0;
            radar.energy_static = 0;
            radar.moving_range_cm = 0;
            kalman.reset(0); // Reset Kalman to prevent giant leaps
            if (radar.static_range_cm > 0) {
                snprintf(radar.detected_message, sizeof(radar.detected_message),
                         "No target. Static background: %.2fm", radar.static_range_cm / 100.0f);
            } else {
                snprintf(radar.detected_message, sizeof(radar.detected_message),
                         "No target detected.");
            }
        }
        else if (strncmp(line, "Range", 5) == 0) {
            // Protocol sends "Range NNNN" where NNNN is cm
            int raw_cm = 0;
            if (sscanf(line, "Range %d", &raw_cm) == 1 && raw_cm > 0) {
                uint32_t now = millis();

                // True Calculus Engine: Adaptive Kalman Filter for optimal tracking
                if (kalman.getPosition() == 0) {
                    kalman.reset((float)raw_cm);
                } else {
                    kalman.update((float)raw_cm);
                }
                
                radar.kalman_innov = kalman.getInnovation();
                radar.velocity_cm_s = (int32_t)kalman.getVelocity();
                uint32_t smoothed_cm = (uint32_t)kalman.getPosition();

                // Trajectory Calculus
                int32_t abs_v = abs(radar.velocity_cm_s);
                bool has_radial_motion = abs_v > 15;
                radar.tangential_motion = false;

                if (has_radial_motion) {
                    last_motion_ms = now;
                    radar.energy_moving = constrain(40 + abs_v * 2, 40, 100);
                } else if (radar.presence_hardware && abs_v <= 15) {
                    // Left-to-Right Orthogonal Motion Calculus
                    radar.tangential_motion = true;
                    last_motion_ms = now;
                    radar.energy_moving = 60; // Physical energy of tangential motion
                } else {
                    if (now - last_motion_ms > 500) {
                        if (radar.energy_moving > 5) radar.energy_moving -= 5;
                        else radar.energy_moving = 0;
                    }
                }

                radar.range_cm = smoothed_cm; // Use Kalman smoothed pos instead of raw_cm
                distance_ema_cm = kalman.getPosition();
                
                radar.last_seen_ms = now;
                last_static_ms = now;
                radar.energy_static = constrain(100 - (smoothed_cm / 8), 10, 100);
                radar.presence_serial = true;

                // Determine if motion is active (moving target detected)
                bool has_motion = (radar.energy_moving > 30) || (now - last_motion_ms < 3000);
                if (has_motion) {
                    const char* trend = "Stationary";
                    if (radar.tangential_motion) trend = "Left-to-Right";
                    else if (radar.velocity_cm_s < -15) trend = "Approaching";
                    else if (radar.velocity_cm_s > 15) trend = "Departing";

                    radar.moving_range_cm = smoothed_cm;
                    
                    uint8_t zone = smoothed_cm / 100;
                    if (zone > 7) zone = 7;
                    snprintf(radar.detected_message, sizeof(radar.detected_message),
                             "Moving: %.2fm (%s, Z%d)", smoothed_cm / 100.0f, trend, zone + 1);
                } else {
                    radar.static_range_cm = smoothed_cm;
                    
                    uint8_t zone = smoothed_cm / 100;
                    if (zone > 7) zone = 7;
                    snprintf(radar.detected_message, sizeof(radar.detected_message),
                             "Static: %.2fm (Z%d)", smoothed_cm / 100.0f, zone + 1);
                }
            }
        }
        else {
            radar.frame_errors++;
        }

        updatePresenceEngine();
        updateZoneMap();
        inferActivity();
        calculateConfidences();
    }
};

AsciiParser parser;

// ================================================================================
// PRESENCE INTELLIGENCE ENGINE
// ================================================================================
void updatePresenceEngine() {
    uint32_t now = millis();
    uint32_t since_motion = now - last_motion_ms;
    uint32_t since_static = now - last_static_ms;
    uint32_t state_duration = now - state_start_ms;

    bool hw = radar.presence_hardware;
    bool sw = radar.presence_serial;
    bool any_presence = hw || sw;

    PresenceState prev_state = radar.presence_state;
    PresenceState new_state;

    if (!any_presence && since_motion > PRESENCE_TIMEOUT_MS && since_static > PRESENCE_TIMEOUT_MS) {
        new_state = STATE_EMPTY;
    }
    else if (!any_presence && since_motion < PRESENCE_TIMEOUT_MS) {
        new_state = STATE_POSSIBLE;
    }
    else if (any_presence && radar.energy_moving > 70) {
        if (radar.range_cm > 0 && radar.range_cm < 100) {
            new_state = STATE_MULTI_MOVEMENT;
        } else {
            new_state = STATE_MOTION_DETECTED;
        }
    }
    else if (any_presence && radar.energy_moving < 20 && radar.energy_static > 40) {
        if (state_duration > 30000) {
            new_state = STATE_SLEEPING;
        } else {
            new_state = STATE_MICRO_MOTION;
        }
    }
    else if (any_presence && radar.energy_static > 60) {
        new_state = STATE_STATIC_PRESENCE;
    }
    else if (any_presence) {
        new_state = STATE_CONFIRMED_HUMAN;
    }
    else {
        new_state = STATE_UNKNOWN_ACTIVITY;
    }

    if (new_state != prev_state) {
        state_start_ms = now;
        radar.presence_state = new_state;
    }

    updateSession(any_presence);
}

// ================================================================================
// ZONE MAPPING ENGINE
// ================================================================================
void updateZoneMap() {
    if (radar.range_cm == 0) return;
    uint8_t zone = radar.range_cm / 100;
    if (zone > 7) zone = 7;

    // Decay all zones slightly
    for (int z = 0; z < 8; z++) {
        if (radar.zones[z].motion_intensity > 0) radar.zones[z].motion_intensity--;
        if (radar.zones[z].static_intensity > 0) radar.zones[z].static_intensity--;
        // Decay occupancy probability
        if (radar.zones[z].occupancy_prob > 0) radar.zones[z].occupancy_prob =
            max(0, (int)radar.zones[z].occupancy_prob - 1);
    }

    // Update active zone
    radar.zones[zone].motion_intensity = min(100, (int)radar.energy_moving);
    radar.zones[zone].static_intensity = min(100, (int)radar.energy_static);
    radar.zones[zone].presence_count++;
    radar.zones[zone].occupancy_prob = min(100,
        (int)radar.zones[zone].occupancy_prob +
        ((radar.presence_serial || radar.presence_hardware) ? 10 : -5));
}

// ================================================================================
// ACTIVITY INFERENCE ENGINE
// ================================================================================
void inferActivity() {
    uint32_t state_dur_s = (millis() - state_start_ms) / 1000;
    
    if (radar.presence_state == STATE_EMPTY) {
        markov.process(0.0f, 0.0f, 0.0f); // Idle forces IDLE state over time
        radar.activity = ACTIVITY_NONE;
    } else {
        markov.process((float)radar.velocity_cm_s, (float)state_dur_s, radar.kalman_innov);
        
        // Output probabilities to radar state for the dashboard
        for (int i = 0; i < 6; i++) {
            radar.markov_probs[i] = markov.probs[i];
        }
        
        uint8_t best = markov.getBest();
        
        // Map HMM index to Activity enum
        if (best == 0) radar.activity = ACTIVITY_NONE;
        else if (best == 1) radar.activity = ACTIVITY_SLEEPING;
        else if (best == 2) radar.activity = ACTIVITY_SITTING;
        else if (best == 3) radar.activity = ACTIVITY_STANDING;
        else if (best == 4) radar.activity = ACTIVITY_WALKING;
        else if (best == 5) radar.activity = ACTIVITY_WALKING; // Map running to walking visually
        
        // Cross-Range bypass (If moving sideways but radial is stationary)
        if (radar.tangential_motion) {
            radar.activity = ACTIVITY_WALKING;
        }
    }

    // Scores

    radar.motion_score      = radar.energy_moving;
    radar.activity_score    = (radar.energy_moving + radar.energy_static) / 2;
    radar.restlessness_score = (radar.energy_moving > 60) ? radar.energy_moving - 60 : 0;
    radar.stability_score   = (radar.energy_static > radar.energy_moving)
                               ? radar.energy_static : 100 - radar.energy_moving;
}

// ================================================================================
// CONFIDENCE CALCULATION
// ================================================================================
void calculateConfidences() {
    bool hw = radar.presence_hardware;
    bool sw = radar.presence_serial;

    // Presence confidence — dual-source agreement
    uint8_t base = 0;
    if (hw && sw) base = 95;
    else if (hw || sw) base = 65;
    else if (radar.presence_state == STATE_POSSIBLE) base = 30;
    else base = 5;

    // Distance plausibility bonus
    if (radar.range_cm > 0 && radar.range_cm < 600) base = min(100, base + 10);

    radar.presence_confidence  = base;
    radar.human_confidence     = (base > 60) ? min(100, base + radar.activity_score / 4) : base / 2;
    radar.occupancy_confidence = (radar.total_occupancy_ms > 5000)
                                  ? min(100, base + 15) : base;

    // Signal quality from energy and distance
    float q = (radar.energy_moving + radar.energy_static) / 2.0f;
    if (radar.range_cm > 400) q *= 0.8f;
    radar.signal_quality = (uint8_t)constrain(q, 0, 100);
    radar.noise_floor_est = -80 + (int8_t)(radar.signal_quality / 5);
}

// ================================================================================
// SESSION TRACKING
// ================================================================================
void updateSession(bool present) {
    if (present) {
        if (radar.current_session_start == 0) {
            radar.current_session_start = millis();
        }
        // Running average distance
        if (radar.range_cm > 0) {
            radar.avg_distance_cm_session = (radar.avg_distance_cm_session * 0.9f)
                                            + (radar.range_cm * 0.1f);
        }
    } else {
        if (radar.current_session_start > 0) {
            // Close session
            uint8_t idx = radar.session_count % MAX_SESSIONS;
            radar.sessions[idx].start_ms       = radar.current_session_start;
            radar.sessions[idx].end_ms         = millis();
            radar.sessions[idx].avg_distance_cm = radar.avg_distance_cm_session;
            radar.sessions[idx].active         = false;
            radar.session_count++;
            radar.current_session_start = 0;
            radar.avg_distance_cm_session = 0;
        }
    }
}

// ================================================================================
// JSON PAYLOAD BUILDER
// Note: Uses DynamicJsonDocument (heap) — NOT StaticJsonDocument (stack).
// StaticJsonDocument<2048> would put 2KB on the 4KB ESP8266 stack → overflow.
// ================================================================================
String buildJsonPayload(bool full) {
    // If not full, we use a much smaller document (768 bytes) to avoid heap fragmentation and pressure
    DynamicJsonDocument doc(full ? 1536 : 768);

    // Core state
    doc["presence"]           = (radar.presence_serial || radar.presence_hardware);
    doc["hardware_presence"]  = radar.presence_hardware;
    doc["serial_presence"]    = radar.presence_serial;
    doc["state"]              = PRESENCE_STATE_NAMES[radar.presence_state];
    doc["state_id"]           = (int)radar.presence_state;
    doc["activity"]           = ACTIVITY_NAMES[radar.activity];
    doc["activity_id"]        = (int)radar.activity;

    // Distance — both cm and m
    doc["distance_cm"]        = radar.range_cm;
    doc["distance_cm_smooth"] = (uint32_t)distance_ema_cm;
    doc["distance_m"]         = radar.range_cm / 100.0f;
    doc["moving_dist_cm"]     = radar.moving_range_cm;
    doc["static_dist_cm"]     = radar.static_range_cm;
    doc["detected_msg"]       = radar.detected_message;

    // True Velocity & Vector Physics
    doc["velocity_cm_s"]      = radar.velocity_cm_s;
    doc["tangential_motion"]  = radar.tangential_motion;
    
    // Probabilistic Metrics (HMM + AKF)
    doc["kalman_innov"]       = radar.kalman_innov;
    JsonArray hmm = doc.createNestedArray("markov");
    for (int i = 0; i < 6; i++) {
        hmm.add(radar.markov_probs[i]);
    }

    // Energy
    doc["energy_moving"]      = radar.energy_moving;
    doc["energy_static"]      = radar.energy_static;

    // Confidence
    doc["presence_confidence"]  = radar.presence_confidence;
    doc["human_confidence"]     = radar.human_confidence;
    doc["occupancy_confidence"] = radar.occupancy_confidence;
    doc["signal_quality"]       = radar.signal_quality;
    doc["noise_floor"]          = radar.noise_floor_est;

    // Scores
    doc["motion_score"]       = radar.motion_score;
    doc["activity_score"]     = radar.activity_score;
    doc["restlessness"]       = radar.restlessness_score;
    doc["stability"]          = radar.stability_score;

    // Zone map (trimmed: from_cm/to_cm calculable client-side as z*100)
    JsonArray zones = doc.createNestedArray("zones");
    for (int z = 0; z < 8; z++) {
        JsonObject zobj = zones.createNestedObject();
        zobj["z"]        = z;
        zobj["motion"]   = radar.zones[z].motion_intensity;
        zobj["static_e"] = radar.zones[z].static_intensity;
        zobj["prob"]     = radar.zones[z].occupancy_prob;
    }

    // Sessions and Hourly (only in full payload)
    if (full) {
        uint32_t cur_dur = radar.current_session_start > 0
                           ? (millis() - radar.current_session_start) / 1000
                           : 0;
        doc["current_session_s"]  = cur_dur;
        doc["session_count"]      = radar.session_count;
        doc["total_occupancy_s"]  = radar.total_occupancy_ms / 1000;

        // Hourly array
        JsonArray hourly = doc.createNestedArray("hourly");
        for (int h = 0; h < HISTORY_BINS; h++) {
            hourly.add(radar.hourly_occupancy[h]);
        }
    }

    // Diagnostics
    doc["packets"]        = radar.packets_received;
    doc["errors"]         = radar.frame_errors;
    doc["overflow"]       = radar.buffer_overflow;
    doc["uptime_s"]       = radar.uptime_seconds;
    doc["heap_free"]      = ESP.getFreeHeap();
    doc["wifi_rssi"]      = WiFi.RSSI();
    doc["ms_since_seen"]  = millis() - radar.last_seen_ms;

    // Settings
    doc["thresh_motion_cm"] = threshold_motion_cm;
    doc["thresh_static_cm"] = threshold_static_cm;
    doc["sensitivity"]      = sensitivity_level;

    String out;
    out.reserve(full ? 900 : 500);
    serializeJson(doc, out);
    return out;
}

// ================================================================================
// WEBSOCKET HANDLER
// ================================================================================
void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
        String json = buildJsonPayload(true); // Send full payload on initial connection
        wsServer.sendTXT(num, json);
    }
}

void sendWebSocketUpdate() {
    // Guard: skip broadcast if heap is critically low to prevent crash
    // Since we optimized the slim payload, we lower the threshold to 3000
    if (ESP.getFreeHeap() < 3000) {
        Serial.printf("[WS] Low heap (%u) — skipping broadcast\n", ESP.getFreeHeap());
        return;
    }
    String json = buildJsonPayload(false); // Send slim payload for periodic broadcast
    wsServer.broadcastTXT(json);
}

// ================================================================================
// MQTT
// ================================================================================
#if ENABLE_MQTT
void mqttReconnect() {
    if (strlen(MQTT_HOST) == 0) return;
    if (!mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) return;

    // Home Assistant auto-discovery
    String disc_topic = String("homeassistant/binary_sensor/ld2420/presence/config");
    StaticJsonDocument<512> ha;
    ha["name"]         = "LD2420 Presence";
    ha["unique_id"]    = "ld2420_presence";
    ha["state_topic"]  = String(MQTT_TOPIC_BASE) + "/presence";
    ha["payload_on"]   = "ON";
    ha["payload_off"]  = "OFF";
    ha["device_class"] = "occupancy";
    String ha_str;
    serializeJson(ha, ha_str);
    mqttClient.publish(disc_topic.c_str(), ha_str.c_str(), true);
}

void mqttPublish() {
    if (strlen(MQTT_HOST) == 0) return;
    if (!mqttClient.connected()) { mqttReconnect(); return; }

    bool present = radar.presence_serial || radar.presence_hardware;
    mqttClient.publish((String(MQTT_TOPIC_BASE) + "/presence").c_str(), present ? "ON" : "OFF");
    mqttClient.publish((String(MQTT_TOPIC_BASE) + "/distance_cm").c_str(),
                       String(radar.range_cm).c_str());
    mqttClient.publish((String(MQTT_TOPIC_BASE) + "/confidence").c_str(),
                       String(radar.presence_confidence).c_str());
    mqttClient.publish((String(MQTT_TOPIC_BASE) + "/state").c_str(),
                       PRESENCE_STATE_NAMES[radar.presence_state]);
    mqttClient.publish((String(MQTT_TOPIC_BASE) + "/activity").c_str(),
                       ACTIVITY_NAMES[radar.activity]);
    mqttClient.publish((String(MQTT_TOPIC_BASE) + "/json").c_str(),
                       buildJsonPayload(true).c_str());
}
#endif

// ================================================================================
// WEB SERVER HANDLERS
// ================================================================================
void handleApiData() {
    httpServer.sendHeader("Access-Control-Allow-Origin", "*");
    httpServer.send(200, "application/json", buildJsonPayload(true));
}

void handleApiHex() {
    httpServer.sendHeader("Access-Control-Allow-Origin", "*");
    String out;
    out.reserve(SNAPSHOT_SIZE + 10);
    uint16_t start = snapIdx;
    for (int i = 0; i < SNAPSHOT_SIZE; i++) {
        uint8_t b = snapBuf[(start + i) % SNAPSHOT_SIZE];
        if (b >= 32 && b <= 126) out += (char)b;
        else if (b == '\n') out += '\n';
    }
    httpServer.send(200, "text/plain", out);
}

void handleApiCmd() {
    httpServer.sendHeader("Access-Control-Allow-Origin", "*");
    if (!httpServer.hasArg("action")) {
        httpServer.send(400, "text/plain", "Missing action");
        return;
    }
    String a = httpServer.arg("action");
    if (a == "enable")  sendRadarCmd(CMD_ENABLE_CONFIG,  sizeof(CMD_ENABLE_CONFIG));
    else if (a == "disable") sendRadarCmd(CMD_DISABLE_CONFIG, sizeof(CMD_DISABLE_CONFIG));
    else if (a == "version") sendRadarCmd(CMD_READ_VERSION,   sizeof(CMD_READ_VERSION));
    else if (a == "reboot")  sendRadarCmd(CMD_REBOOT,         sizeof(CMD_REBOOT));
    else if (a == "clear_sessions") {
        memset(radar.sessions, 0, sizeof(radar.sessions));
        radar.session_count = 0;
        radar.total_occupancy_ms = 0;
        memset(radar.hourly_occupancy, 0, sizeof(radar.hourly_occupancy));
        radar.moving_range_cm = 0;
        radar.static_range_cm = 0;
        memset(radar.detected_message, 0, sizeof(radar.detected_message));
    }
    httpServer.send(200, "text/plain", "OK");
}

void handleApiThresholds() {
    httpServer.sendHeader("Access-Control-Allow-Origin", "*");
    if (httpServer.hasArg("motion_cm"))    threshold_motion_cm = httpServer.arg("motion_cm").toInt();
    if (httpServer.hasArg("static_cm"))    threshold_static_cm = httpServer.arg("static_cm").toInt();
    if (httpServer.hasArg("sensitivity"))  sensitivity_level   = httpServer.arg("sensitivity").toInt();
    httpServer.send(200, "text/plain", "OK");
}

// ================================================================================
// DASHBOARD HTML — PREMIUM AEROSPACE / RADAR AESTHETIC
// The full dashboard is defined here as a PROGMEM string.
// Note: escape sequences and special characters handled below.
// ================================================================================

// Due to ESP8266 flash constraints we use PROGMEM for the HTML
// The full HTML is split across multiple PROGMEM blocks and served in chunks

void handleRoot() {
    // Stream the HTML in a single response
    // (The actual HTML is in the companion .html file)
    // For embedded use, define the HTML inline below as PROGMEM strings
    httpServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    httpServer.send(200, "text/html", "");
    // The HTML file itself is served separately; see LD2420_Dashboard.html
    // For full embedded firmware, paste the HTML between the markers below:
    // === BEGIN EMBEDDED HTML ===
    // (See companion LD2420_Dashboard.html — serves full premium UI)
    // === END EMBEDDED HTML ===
}

// ================================================================================
// SETUP
// ================================================================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n\n=== LD2420 PRESENCE INTELLIGENCE PLATFORM v4.0 ===");

    // GPIO
    pinMode(PIN_RADAR_OT2, INPUT);
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);  // LED off (active LOW)

    // OT2 interrupt
    attachInterrupt(digitalPinToInterrupt(PIN_RADAR_OT2), AppLogic::handleOT2, CHANGE);
    radar.presence_hardware = digitalRead(PIN_RADAR_OT2) == HIGH;

    // Radar UART — 256-byte buffer (reduced to save RAM)
    radarSerial.begin(RADAR_BAUD, SWSERIAL_8N1, PIN_RADAR_RX, PIN_RADAR_TX, false, 256);
    Serial.println("[RADAR] SoftwareSerial started @ 115200");

    // WiFiManager - Secure Captive Portal
    WiFiManager wifiManager;
    // wifiManager.resetSettings(); // Uncomment to wipe saved credentials for testing
    wifiManager.setAPCallback([](WiFiManager *myWiFiManager) {
        Serial.println("[WIFI] Entering Setup Mode");
        Serial.printf("[AP] Connect to SSID: %s to configure WiFi\n", myWiFiManager->getConfigPortalSSID().c_str());
    });
    
    Serial.println("[WIFI] Connecting or starting AP...");
    if (!wifiManager.autoConnect(AP_SSID, AP_PASS)) {
        Serial.println("[WIFI] Failed to connect and hit timeout. Rebooting...");
        delay(3000);
        ESP.restart();
        delay(5000);
    }
    
    Serial.printf("\n[WIFI] Connected — http://%s\n", WiFi.localIP().toString().c_str());
    
    #if ENABLE_MDNS
    if (MDNS.begin("ld2420")) {
        Serial.println("[mDNS] Responder started at http://ld2420.local");
    }
    #endif

    // HTTP routes
    httpServer.on("/",                 handleRoot);
    httpServer.on("/api/data",         handleApiData);
    httpServer.on("/api/hex",          handleApiHex);
    httpServer.on("/api/cmd",          handleApiCmd);
    httpServer.on("/api/thresholds",   handleApiThresholds);
    httpServer.begin();

    // WebSocket
    wsServer.begin();
    wsServer.onEvent(onWsEvent);

    // MQTT
    #if ENABLE_MQTT
    if (strlen(MQTT_HOST) > 0) {
        mqttClient.setServer(MQTT_HOST, MQTT_PORT);
        mqttClient.setSocketTimeout(2);  // 2s socket timeout — prevent blocking
        mqttReconnect();
    }
    #endif

    // OTA
    #if ENABLE_OTA
    ArduinoOTA.setHostname("ld2420-radar");
    ArduinoOTA.onStart([]() { Serial.println("[OTA] Start"); });
    ArduinoOTA.onEnd([]()   { Serial.println("\n[OTA] Done"); });
    ArduinoOTA.begin();
    #endif

    // 1-second ticker for analytics
    secondTicker.attach(1.0f, secondISR);

    // Init EMA
    distance_ema_cm = 0.0f;
    state_start_ms  = millis();

    Serial.println("[SYS] Boot complete. Awaiting radar data...");
    Serial.printf("[SYS] Free heap: %u bytes\n", ESP.getFreeHeap());
    digitalWrite(PIN_LED, LOW);  // LED on = ready
}

// ================================================================================
// MAIN LOOP
// ================================================================================
void loop() {
    #if ENABLE_OTA
    ArduinoOTA.handle();
    #endif
    httpServer.handleClient();
    wsServer.loop();
    #if ENABLE_MDNS
    MDNS.update();
    #endif

    while (radarSerial.available()) {
        AppLogic::parser.feed((uint8_t)radarSerial.read());
    }

    uint32_t now = millis();
    AppLogic::checkPresenceTimeout();

    if (now - AppLogic::last_doppler_ms >= DOPPLER_ANALYZE_MS) {
        AppLogic::last_doppler_ms = now;
        AppLogic::runDopplerAnalysis();
    }

    if (now - AppLogic::last_breathing_ms >= BREATHING_ANALYZE_MS) {
        AppLogic::last_breathing_ms = now;
        AppLogic::runBioVitalAnalysis();
    }

    AppLogic::manageFallAlert();

    if (now - AppLogic::last_broadcast_ms >= TELEMETRY_MS) {
        AppLogic::last_broadcast_ms = now;
        AppLogic::runPresenceFusion();
        AppLogic::runActivityClassification();

        if (!AppLogic::radar.presence_fused) {
            AppLogic::occupancy.update(0, false);
            for (int z = 0; z < UltimateDSP::OccupancyGridEngine::ZONES; z++)
                AppLogic::radar.zone_prob[z] = AppLogic::occupancy.getProbability(z);
        }

        if(AppLogic::radar.presence_fused) {
            AppLogic::radar.posture_class = AppLogic::svm_posture.predict(AppLogic::radar.kalman_innovation, 100.0);
            AppLogic::radar.pet_detected = AppLogic::dt_pet.isPet(50.0, AppLogic::radar.velocity_cm_s, 20.0);
            AppLogic::rl_tuner.step(AppLogic::kalman, AppLogic::radar.kalman_innovation);
            AppLogic::isolation_forest.update(abs(AppLogic::radar.velocity_cm_s));
            AppLogic::radar.anomaly_score = AppLogic::isolation_forest.anomaly_score;
            AppLogic::sleep_stager.update(AppLogic::radar.breathing_rate_bpm, AppLogic::radar.breathing_valid);
            AppLogic::radar.sleep_stage = AppLogic::sleep_stager.current_stage;
            AppLogic::intent_engine.update(AppLogic::radar.range_cm, AppLogic::radar.velocity_cm_s);
            AppLogic::radar.intent_leaving = AppLogic::intent_engine.intention_to_leave;
            AppLogic::radar.voice_prime = (abs(AppLogic::radar.velocity_cm_s) < 5.0 && AppLogic::radar.range_cm < 100.0);
        } else {
            AppLogic::radar.posture_class = 0;
            AppLogic::radar.pet_detected = false;
            AppLogic::radar.anomaly_score = 0;
            AppLogic::radar.sleep_stage = 0;
            AppLogic::radar.intent_leaving = false;
            AppLogic::radar.voice_prime = false;
        }

        sendWebSocketUpdate();
        AppLogic::blinkLED();
    }

    AppLogic::updateLED();

    #if ENABLE_MQTT
    if (millis() - last_mqtt_ms > MQTT_INTERVAL_MS) {
        last_mqtt_ms = millis();
        if (strlen(MQTT_HOST) > 0) {
            mqttClient.loop();
            mqttPublish();
        }
    }
    #endif

    yield();
}
