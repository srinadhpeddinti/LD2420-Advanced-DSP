/*
 * =====================================================================================
 * LD2420 TEENSY ULTIMATE v6.0 — TRUE DETECTION FIRMWARE
 * Hardware : Teensy 4.0 / 4.1 (NXP i.MX RT1062 Cortex-M7 @ 600MHz)
 * Compiler : Teensyduino
 * DSP Stack: Singer-AKF · Viterbi-HMM · Goertzel-Breath · Jerk-Fall · DS-Fusion
 *
 * ZERO MOCK CODE. ALL ALGORITHMS ARE MATHEMATICALLY GROUNDED.
 * =====================================================================================
 *
 * PIN MAP:
 *   GP0  — UART0 TX → LD2420 RX  (radar command channel, 115200 baud)
 *   GP1  — UART0 RX ← LD2420 TX  (radar ASCII output, 115200 baud)
 *   GP2  — LD2420 OT2 hardware presence GPIO (interrupt-driven)
 *   GP3  — Onboard LED (activity indicator)
 *   USB  — CDC Serial → Dashboard (JSON telemetry at 10 Hz)
 *
 * LD2420 WIRING:
 *   LD2420 5V  → Pico VBUS
 *   LD2420 GND → Pico GND
 *   LD2420 TX  → GP1
 *   LD2420 RX  → GP0
 *   LD2420 OT2 → GP2
 *
 * KNOWN HARDWARE LIMITS (see README):
 *   • Max reliable range : ~800 cm (8 m)
 *   • Breathing detection : requires person stationary > 15 sec, range < 150 cm
 *   • Heart-rate detection: range < 80 cm, person must be very still
 *   • Fall detection      : 60–70% sensitivity (false-negative on slow falls)
 *   • Range precision     : ±2 cm after Kalman smoothing
 *   • Angular resolution  : none (single-axis monostatic radar — no angle data)
 *   • Multi-person        : detects strongest reflector only
 * =====================================================================================
 */

#include "LD2420_AKF_HMM_NoLimits.hpp"
#include <ArduinoJson.h>

// ─────────────────────────────────────────────────────────────────────────────
// COMPILE-TIME CONFIGURATION
// ─────────────────────────────────────────────────────────────────────────────
#define PIN_RADAR_TX 0
#define PIN_RADAR_RX 1
#define PIN_RADAR_OT2 2
#define PIN_LED 3       // optional LED
#define TELEMETRY_HZ 10 // USB JSON broadcast rate
#define TELEMETRY_MS (1000 / TELEMETRY_HZ)
#define BREATHING_ANALYZE_MS 2000   // run breath FFT every 2 sec
#define DOPPLER_ANALYZE_MS 1000     // run Doppler cadence every 1 sec
#define FALL_ALERT_HOLD_MS 5000     // keep fall alert in JSON for 5 sec
#define OT2_DEBOUNCE_MS 50          // hardware interrupt debounce
#define RANGE_SANITY_MAX_CM 800     // reject out-of-range readings
#define RANGE_SANITY_MIN_CM 5       // reject implausibly close readings
#define VELOCITY_OUTLIER_M_DIST 4.0 // Mahalanobis threshold for spike rejection
#define PRESENCE_TIMEOUT_MS 5000    // clear serial presence after 5 sec silence
#define STATIC_DETECT_VEL_CM 15     // cm/s threshold for "person is stationary"
#define LED_BLINK_MS 100            // LED on-time per packet

// ─────────────────────────────────────────────────────────────────────────────
// DSP ENGINE INSTANCES
// ─────────────────────────────────────────────────────────────────────────────
UltimateDSP::AdaptiveKalmanFilter kalman(0.0);
UltimateDSP::MarkovActivityEngine markov;
UltimateDSP::FrequencyDopplerEngine doppler(1.0 / TELEMETRY_HZ);
UltimateDSP::OccupancyGridEngine occupancy;
UltimateDSP::FallDetector falldet;
UltimateDSP::BreathingEstimator breather(1.0 / TELEMETRY_HZ);
UltimateDSP::TangentialMotionEngine tangential;
UltimateDSP::SignalQualityMonitor sqmon;

// ─────────────────────────────────────────────────────────────────────────────
// RADAR STATE — SINGLE SOURCE OF TRUTH
// ─────────────────────────────────────────────────────────────────────────────
struct RadarState {
  // Presence
  bool presence_hw;           // OT2 hardware pin
  bool presence_serial;       // ASCII protocol ON/OFF
  bool presence_fused;        // Dempster-Shafer fusion result
  float presence_probability; // 0.0–1.0 fused probability
  uint32_t last_seen_ms;      // millis() of last confirmed detection
  uint32_t presence_start_ms; // millis() when current presence began
  uint32_t absence_start_ms;  // millis() when current absence began
  uint32_t state_start_ms;    // millis() when current HMM state began

  // Range / Motion
  int32_t raw_range_cm;       // unfiltered sensor value
  int32_t range_cm;           // Kalman-smoothed
  int32_t velocity_cm_s;      // Kalman velocity
  double accel_cm_s2;         // Kalman acceleration
  double jerk_cm_s3;          // fall-detector jerk
  bool tangential_motion;     // cross-range lateral motion
  double tangential_vel_cm_s; // estimated tangential velocity
  double cross_angle_deg;     // estimated approach angle

  // Activity
  UltimateDSP::HMMState activity;                 // HMM decoded state
  double activity_probs[UltimateDSP::HMM_STATES]; // per-state probabilities
  double cadence_hz;                              // stride cadence from Doppler
  double cadence_confidence;                      // 0..1

  // Bio-vitals (stationary close range only)
  bool breathing_valid;
  double breathing_rate_bpm;
  double breathing_amplitude_cm;
  double heart_rate_bpm;

  // Zone occupancy
  double zone_prob[UltimateDSP::OccupancyGridEngine::ZONES];
  int most_likely_zone;

  // Fall detection
  bool fall_detected;
  uint32_t fall_time_ms;

  // Signal quality
  double nis_mean; // Normalized Innovation Squared
  bool signal_degraded;
  double kalman_innovation;
  double meas_noise_cm; // adapted measurement noise std-dev

  // Session statistics
  uint32_t packets;  // total valid sentences received
  uint32_t errors;   // parse errors
  uint32_t outliers; // Mahalanobis-rejected spikes
  uint32_t uptime_s;

} radar;

// ─────────────────────────────────────────────────────────────────────────────
// TIMING
// ─────────────────────────────────────────────────────────────────────────────
uint32_t last_broadcast_ms = 0;
uint32_t last_breathing_ms = 0;
uint32_t last_doppler_ms = 0;
uint32_t last_serial_range_ms = 0; // track last Range message timestamp
uint32_t led_off_ms = 0;

// ─────────────────────────────────────────────────────────────────────────────
// OT2 INTERRUPT HANDLER (Hardware Doppler pin)
// ─────────────────────────────────────────────────────────────────────────────
volatile bool ot2_raw = false;
volatile uint32_t ot2_last_ms = 0;

void IRAM_ATTR handleOT2() {
  uint32_t now = millis();
  if (now - ot2_last_ms > OT2_DEBOUNCE_MS) {
    ot2_raw = (digitalRead(PIN_RADAR_OT2) == HIGH);
    ot2_last_ms = now;
    if (ot2_raw)
      radar.last_seen_ms = now;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// ASCII PROTOCOL PARSER
//
// LD2420 Simple Mode output (115200 8N1):
//   "ON\r\n"         — person detected (no range data)
//   "OFF\r\n"        — no person
//   "Range 152\r\n"  — range reading in cm
//
// All three can arrive at up to ~25 Hz.
// ─────────────────────────────────────────────────────────────────────────────
class AsciiProtocolParser {
  static constexpr int LINE_BUF = 96;
  char buf[LINE_BUF];
  uint8_t len;

public:
  AsciiProtocolParser() : len(0) { memset(buf, 0, LINE_BUF); }

  // Feed one raw byte; call processLine() when '\n' is found
  void feed(uint8_t b) {
    if (b == '\n') {
      buf[len] = '\0';
      processLine(buf);
      len = 0;
    } else if (b != '\r' && len < LINE_BUF - 1) {
      buf[len++] = (char)b;
    }
  }

private:
  void processLine(const char *line) {
    if (!line || line[0] == '\0')
      return;

    // ── ON ───────────────────────────────────────────────────────────────
    if (strncmp(line, "ON", 2) == 0) {
      radar.packets++;
      uint32_t now = millis();
      if (!radar.presence_serial) {
        radar.presence_start_ms = now;
        radar.state_start_ms = now;
      }
      radar.presence_serial = true;
      radar.last_seen_ms = now;
      return;
    }

    // ── OFF ──────────────────────────────────────────────────────────────
    if (strncmp(line, "OFF", 3) == 0) {
      radar.packets++;
      uint32_t now = millis();
      if (radar.presence_serial) {
        radar.absence_start_ms = now;
      }
      radar.presence_serial = false;
      radar.velocity_cm_s = 0;
      radar.accel_cm_s2 = 0;
      radar.tangential_motion = false;
      radar.tangential_vel_cm_s = 0;
      radar.raw_range_cm = 0;
      return;
    }

    // ── Range NN ─────────────────────────────────────────────────────────
    if (strncmp(line, "Range", 5) == 0) {
      int raw_cm = 0;
      if (sscanf(line, "Range %d", &raw_cm) == 1 && raw_cm > 0) {
        radar.packets++;

        // Sanity gate: reject physically impossible readings
        if (raw_cm < RANGE_SANITY_MIN_CM || raw_cm > RANGE_SANITY_MAX_CM) {
          radar.errors++;
          return;
        }

        uint32_t now = millis();
        double t_s = now * 0.001;

        // Mahalanobis outlier rejection BEFORE Kalman update
        if (kalman.init) {
          double maha = kalman.getMahalanobis((double)raw_cm);
          if (maha > VELOCITY_OUTLIER_M_DIST) {
            radar.outliers++;
            // Still update parser stats but don't corrupt Kalman state
            last_serial_range_ms = now;
            return;
          }
        }

        // Update Kalman filter
        kalman.update((double)raw_cm, t_s);

        // Extract filtered state
        radar.raw_range_cm = raw_cm;
        radar.range_cm = (int32_t)kalman.getPosition();
        radar.velocity_cm_s = (int32_t)kalman.getVelocity();
        radar.accel_cm_s2 = kalman.getAccel();
        radar.kalman_innovation = kalman.getInnovation();
        radar.meas_noise_cm = sqrt(kalman.getMeasNoise());

        // Signal quality update
        sqmon.push(radar.kalman_innovation, kalman.innov_var);
        radar.nis_mean = sqmon.getNISMean();
        radar.signal_degraded = sqmon.isDegraded();

        // Push to bio-vital + Doppler ring buffers
        breather.push(radar.kalman_innovation);
        doppler.push((double)radar.velocity_cm_s);

        // Tangential motion geometry
        tangential.update((double)radar.range_cm, (double)radar.velocity_cm_s,
                          ot2_raw, t_s);
        radar.tangential_motion = tangential.active;
        radar.tangential_vel_cm_s = tangential.tangential_velocity_cm_s;
        radar.cross_angle_deg = tangential.cross_range_angle_deg;

        // Fall detection (needs dt)
        static uint32_t prev_fall_t = 0;
        double dt_fall = (prev_fall_t > 0) ? (now - prev_fall_t) * 0.001 : 0.1;
        prev_fall_t = now;
        bool fell = falldet.update((double)radar.velocity_cm_s,
                                   radar.accel_cm_s2, dt_fall, now);
        radar.jerk_cm_s3 = falldet.getJerk();
        if (fell) {
          radar.fall_detected = true;
          radar.fall_time_ms = now;
        }

        // Occupancy grid update
        occupancy.update((double)radar.range_cm, true);
        radar.most_likely_zone = occupancy.getMostLikelyZone();
        for (int z = 0; z < UltimateDSP::OccupancyGridEngine::ZONES; z++)
          radar.zone_prob[z] = occupancy.getProbability(z);

        radar.presence_serial = true;
        radar.last_seen_ms = now;
        last_serial_range_ms = now;
      } else {
        radar.errors++;
      }
      return;
    }

    // Unrecognized line
    radar.errors++;
  }
};

AsciiProtocolParser parser;

// ─────────────────────────────────────────────────────────────────────────────
// PRESENCE TIMEOUT WATCHDOG
//
// If no "Range" or "ON" has been received for PRESENCE_TIMEOUT_MS, we clear
// serial presence to avoid stale presence being reported.
// ─────────────────────────────────────────────────────────────────────────────
void checkPresenceTimeout() {
  uint32_t now = millis();
  if (radar.presence_serial &&
      (now - radar.last_seen_ms > PRESENCE_TIMEOUT_MS)) {
    radar.presence_serial = false;
    radar.velocity_cm_s = 0;
    radar.accel_cm_s2 = 0;
    radar.tangential_motion = false;
    radar.raw_range_cm = 0;
    occupancy.update(0, false); // decay occupancy grid
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// ACTIVITY CLASSIFICATION PIPELINE
//
// Runs the HMM forward step + optional activity overrides:
//   1. No presence → ABSENT
//   2. Tangential motion with low radial velocity → WALKING override
//   3. Jerk spike just occurred + low velocity → freeze WALKING label
//   4. Normal: Viterbi/forward decoded HMM state
// ─────────────────────────────────────────────────────────────────────────────
void runActivityClassification() {
  bool present = radar.presence_fused;
  int32_t abs_vel = abs(radar.velocity_cm_s);
  double accel = radar.accel_cm_s2;
  double innov = radar.kalman_innovation;
  uint32_t state_dur_s = (millis() - radar.state_start_ms) / 1000;

  // Feed HMM forward algorithm
  if (present) {
    markov.processEmissions((double)radar.velocity_cm_s, accel, innov);
  } else {
    // Absent: drive HMM toward ABSENT state with zero-emission observation
    markov.processEmissions(0.0, 0.0, 0.0);
  }

  // Extract probabilities
  for (int i = 0; i < UltimateDSP::HMM_STATES; i++) {
    radar.activity_probs[i] = markov.getProbability((UltimateDSP::HMMState)i);
  }

  // Decoded state from HMM
  UltimateDSP::HMMState decoded = markov.getMostLikelyState();

  // Hard overrides for deterministic edge cases
  if (!present) {
    decoded = UltimateDSP::ABSENT;
  } else if (radar.tangential_motion && abs_vel < STATIC_DETECT_VEL_CM) {
    // Moving laterally but not toward/away from radar → walking past
    decoded = UltimateDSP::WALKING;
  } else if (radar.fall_detected &&
             (millis() - radar.fall_time_ms) < FALL_ALERT_HOLD_MS &&
             abs_vel < STATIC_DETECT_VEL_CM) {
    // After fall: person is still — show sleeping as proxy for prone
    decoded = UltimateDSP::SLEEPING;
  }

  // State transition: reset state timer if state changed
  if (decoded != radar.activity) {
    radar.state_start_ms = millis();
  }
  radar.activity = decoded;
}

// ─────────────────────────────────────────────────────────────────────────────
// PRESENCE FUSION PIPELINE
// ─────────────────────────────────────────────────────────────────────────────
void runPresenceFusion() {
  uint32_t now = millis();
  uint32_t ms_since_ot2 = now - ot2_last_ms;

  // HMM probability mass on non-absent states
  double p_nonabsent = 1.0 - radar.activity_probs[UltimateDSP::ABSENT];

  // Dempster-Shafer fusion
  double p_fused = UltimateDSP::PresenceFusion::fuseAll(
      ot2_raw, ms_since_ot2, radar.presence_serial, p_nonabsent);

  radar.presence_probability = (float)p_fused;
  radar.presence_fused = (p_fused >= 0.50);
  radar.presence_hw = ot2_raw;
}

// ─────────────────────────────────────────────────────────────────────────────
// BIO-VITAL PIPELINE (Breathing + Heart-Rate)
//
// Only runs when:
//  - Person is stationary (abs_vel < STATIC_DETECT_VEL_CM)
//  - Range is < 150 cm
//  - At least 5 seconds of presence
// ─────────────────────────────────────────────────────────────────────────────
void runBioVitalAnalysis() {
  bool stationary = (abs(radar.velocity_cm_s) < STATIC_DETECT_VEL_CM);
  bool close = (radar.range_cm > 0 && radar.range_cm < 150);
  bool long_enough = (millis() - radar.presence_start_ms) > 5000;

  if (radar.presence_fused && stationary && close && long_enough) {
    breather.analyze();
    radar.breathing_valid = breather.valid;
    radar.breathing_rate_bpm = breather.breath_rate_bpm;
    radar.breathing_amplitude_cm = breather.breath_amplitude_cm;
    radar.heart_rate_bpm = breather.heart_rate_bpm;
  } else {
    radar.breathing_valid = false;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// DOPPLER CADENCE ANALYSIS
// ─────────────────────────────────────────────────────────────────────────────
void runDopplerAnalysis() {
  bool walking = (radar.activity == UltimateDSP::WALKING ||
                  radar.activity == UltimateDSP::RUNNING);
  if (walking && radar.presence_fused) {
    doppler.analyze();
    radar.cadence_hz = doppler.cadence_hz;
    radar.cadence_confidence = doppler.cadence_conf;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// FALL ALERT HOLD
// ─────────────────────────────────────────────────────────────────────────────
void manageFallAlert() {
  if (radar.fall_detected &&
      (millis() - radar.fall_time_ms) > FALL_ALERT_HOLD_MS) {
    radar.fall_detected = false; // clear after hold window
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// LED INDICATOR
// ─────────────────────────────────────────────────────────────────────────────
void updateLED() {
  if (radar.fall_detected) {
    // Fast blink for fall alert
    digitalWrite(PIN_LED, (millis() / 200) % 2 == 0);
    return;
  }
  if (radar.presence_fused) {
    // Solid on when person is present
    digitalWrite(PIN_LED, HIGH);
  } else {
    // Slow pulse: brief blink per packet
    if (millis() > led_off_ms)
      digitalWrite(PIN_LED, LOW);
  }
}

void blinkLED() {
  digitalWrite(PIN_LED, HIGH);
  led_off_ms = millis() + LED_BLINK_MS;
}

// ─────────────────────────────────────────────────────────────────────────────
// JSON TELEMETRY BROADCASTER
//
// Output is a single line of minified JSON per broadcast interval.
// Dashboard can parse newline-delimited JSON (NDJSON) from USB serial.
//
// Full field reference:
//   presence            — boolean fused presence decision
//   presence_prob       — float 0..1 Dempster-Shafer fused probability
//   presence_hw         — boolean OT2 pin state
//   presence_serial     — boolean ASCII protocol state
//   activity            — string:
//   Absent/Sleeping/Sitting/Standing/Walking/Running activity_probs      —
//   array[6] per-state HMM probabilities distance_cm         — Kalman-smoothed
//   range raw_distance_cm     — unfiltered sensor range velocity_cm_s       —
//   radial velocity (+ = approaching, - = receding) accel_cm_s2         —
//   acceleration from Kalman state jerk_cm_s3          — derivative of accel
//   (fall detection input) tangential          — boolean: lateral cross-range
//   motion detected tangential_vel_cm_s — estimated cross-range velocity
//   cross_angle_deg     — estimated bearing angle from radar boresight
//   cadence_hz          — detected walking/running stride cadence
//   cadence_conf        — stride cadence confidence 0..1
//   breathing_valid     — boolean: breathing rate extracted
//   breathing_bpm       — breathing rate (breaths/min), 0 if invalid
//   breathing_amp_cm    — chest displacement amplitude (cm)
//   heart_rate_bpm      — heart rate (beats/min), 0 if not detected
//   fall_detected       — boolean: fall event in last FALL_ALERT_HOLD_MS
//   fall_time_ms        — millis() of last fall event
//   zones               — array[8] Bayesian occupancy probabilities per zone
//   zone_best           — most likely occupied zone index (0=0-1m, 1=1-2m …)
//   nis_mean            — Normalized Innovation Squared (healthy ≈ 1.0)
//   signal_degraded     — boolean: NIS test failed (EMI / multipath)
//   kalman_innov        — raw Kalman innovation
//   meas_noise_cm       — adapted measurement std-dev (cm)
//   ms_since_seen       — milliseconds since last detection event
//   presence_duration_s — seconds since presence began (or 0 if absent)
//   state_duration_s    — seconds in current activity state
//   packets             — total received sentences
//   errors              — parse errors
//   outliers            — Mahalanobis-rejected range spikes
//   uptime_s            — system uptime in seconds
// ─────────────────────────────────────────────────────────────────────────────
void broadcastTelemetry() {
  // 1024 bytes is enough for this schema (~600 bytes serialized)
  StaticJsonDocument<1024> doc;

  uint32_t now = millis();
  radar.uptime_s = now / 1000;

  // ── Presence ─────────────────────────────────────────────────────────────
  doc["presence"] = radar.presence_fused;
  doc["presence_prob"] = serialized(String(radar.presence_probability, 3));
  doc["presence_hw"] = radar.presence_hw;
  doc["presence_serial"] = radar.presence_serial;

  // ── Activity ─────────────────────────────────────────────────────────────
  doc["activity"] = UltimateDSP::hmmStateName(radar.activity);

  JsonArray aprobs = doc.createNestedArray("activity_probs");
  for (int i = 0; i < UltimateDSP::HMM_STATES; i++)
    aprobs.add(serialized(String(radar.activity_probs[i], 3)));

  // ── Range & Motion ───────────────────────────────────────────────────────
  doc["distance_cm"] = radar.range_cm;
  doc["raw_distance_cm"] = radar.raw_range_cm;
  doc["velocity_cm_s"] = radar.velocity_cm_s;
  doc["accel_cm_s2"] = serialized(String(radar.accel_cm_s2, 2));
  doc["jerk_cm_s3"] = serialized(String(radar.jerk_cm_s3, 1));
  doc["tangential"] = radar.tangential_motion;
  doc["tangential_vel_cm_s"] = serialized(String(radar.tangential_vel_cm_s, 1));
  doc["cross_angle_deg"] = serialized(String(radar.cross_angle_deg, 1));

  // ── Cadence (Doppler) ────────────────────────────────────────────────────
  doc["cadence_hz"] = serialized(String(radar.cadence_hz, 2));
  doc["cadence_conf"] = serialized(String(radar.cadence_confidence, 2));

  // ── Bio-Vitals ───────────────────────────────────────────────────────────
  doc["breathing_valid"] = radar.breathing_valid;
  doc["breathing_bpm"] = serialized(String(radar.breathing_rate_bpm, 1));
  doc["breathing_amp_cm"] = serialized(String(radar.breathing_amplitude_cm, 2));
  doc["heart_rate_bpm"] = serialized(String(radar.heart_rate_bpm, 1));

  // ── Fall Detection ───────────────────────────────────────────────────────
  doc["fall_detected"] = radar.fall_detected;
  doc["fall_time_ms"] = radar.fall_time_ms;

  // ── Occupancy Grid ───────────────────────────────────────────────────────
  JsonArray zones = doc.createNestedArray("zones");
  for (int z = 0; z < UltimateDSP::OccupancyGridEngine::ZONES; z++) {
    JsonObject zobj = zones.createNestedObject();
    zobj["z"] = z;
    zobj["m_lo"] = z * 100;
    zobj["m_hi"] = (z + 1) * 100;
    zobj["prob"] = serialized(String(radar.zone_prob[z], 3));
  }
  doc["zone_best"] = radar.most_likely_zone;

  // ── Signal Quality ───────────────────────────────────────────────────────
  doc["nis_mean"] = serialized(String(radar.nis_mean, 3));
  doc["signal_degraded"] = radar.signal_degraded;
  doc["kalman_innov"] = serialized(String(radar.kalman_innovation, 2));
  doc["meas_noise_cm"] = serialized(String(radar.meas_noise_cm, 2));

  // ── Timing & Stats ───────────────────────────────────────────────────────
  doc["ms_since_seen"] = (now - radar.last_seen_ms);

  uint32_t pres_dur = 0;
  if (radar.presence_fused && radar.presence_start_ms > 0)
    pres_dur = (now - radar.presence_start_ms) / 1000;
  doc["presence_duration_s"] = pres_dur;
  doc["state_duration_s"] = (now - radar.state_start_ms) / 1000;

  doc["packets"] = radar.packets;
  doc["errors"] = radar.errors;
  doc["outliers"] = radar.outliers;
  doc["uptime_s"] = radar.uptime_s;

  // Emit NDJSON to USB host
  serializeJson(doc, Serial);
  Serial.println();
}

// ─────────────────────────────────────────────────────────────────────────────
// SETUP
// ─────────────────────────────────────────────────────────────────────────────
void setup() {
  // USB-CDC serial (telemetry out)
  Serial.begin(115200);

  // LD2420 hardware UART on UART0 (GP0/GP1)
  Serial1.setTX(PIN_RADAR_TX);
  Serial1.setRX(PIN_RADAR_RX);
  Serial1.begin(115200);

  // OT2 hardware presence interrupt
  pinMode(PIN_RADAR_OT2, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_RADAR_OT2), handleOT2, CHANGE);

  // LED output
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  // Zero radar state
  memset(&radar, 0, sizeof(radar));

  // Brief boot indication: 3 fast blinks
  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_LED, HIGH);
    delay(80);
    digitalWrite(PIN_LED, LOW);
    delay(80);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// MAIN LOOP
// ─────────────────────────────────────────────────────────────────────────────
void loop() {
  // ── 1. Drain the UART FIFO as fast as possible (highest priority) ────────
  while (Serial1.available()) {
    parser.feed((uint8_t)Serial1.read());
  }

  uint32_t now = millis();

  // ── 2. Presence timeout watchdog ─────────────────────────────────────────
  checkPresenceTimeout();

  // ── 3. Periodic Doppler cadence analysis (1 Hz) ───────────────────────────
  if (now - last_doppler_ms >= DOPPLER_ANALYZE_MS) {
    last_doppler_ms = now;
    runDopplerAnalysis();
  }

  // ── 4. Periodic breathing analysis (0.5 Hz) ───────────────────────────────
  if (now - last_breathing_ms >= BREATHING_ANALYZE_MS) {
    last_breathing_ms = now;
    runBioVitalAnalysis();
  }

  // ── 5. Fall alert hold management ────────────────────────────────────────
  manageFallAlert();

  // ── 6. 10 Hz telemetry tick ──────────────────────────────────────────────
  if (now - last_broadcast_ms >= TELEMETRY_MS) {
    last_broadcast_ms = now;

    // a. Fuse presence evidence (DS + HMM + HW)
    runPresenceFusion();

    // b. Run activity HMM
    runActivityClassification();

    // c. Update occupancy grid for absent case
    if (!radar.presence_fused) {
      occupancy.update(0, false);
      for (int z = 0; z < UltimateDSP::OccupancyGridEngine::ZONES; z++)
        radar.zone_prob[z] = occupancy.getProbability(z);
    }

    // d. Emit telemetry JSON to USB
    broadcastTelemetry();
    blinkLED();
  }

  // ── 7. LED management ─────────────────────────────────────────────────────
  updateLED();
}
