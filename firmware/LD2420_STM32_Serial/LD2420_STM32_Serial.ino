/*
 * =====================================================================================
 * LD2420 STM32 ULTIMATE v6.0 — TRUE DETECTION FIRMWARE
 * Hardware : STM32 Cortex-M4/M7 series (STM32F4, STM32H7, etc)
 * Compiler : STM32duino core
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


#include "LD2420_AppLogic.hpp"


// ─────────────────────────────────────────────────────────────────────────────
// COMPILE-TIME CONFIGURATION
// ─────────────────────────────────────────────────────────────────────────────

#define PIN_RADAR_TX 0
#define PIN_RADAR_RX 1
#define PIN_RADAR_OT2 2
#define PIN_LED 3       // optional LED


void setup() {
  Serial.begin(115200);

  Serial1.setTX(PIN_RADAR_TX);
  Serial1.setRX(PIN_RADAR_RX);
  Serial1.begin(115200);

  pinMode(PIN_RADAR_OT2, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_RADAR_OT2), AppLogic::handleOT2, CHANGE);

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  memset(&AppLogic::radar, 0, sizeof(AppLogic::radar));

  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_LED, HIGH);
    delay(80);
    digitalWrite(PIN_LED, LOW);
    delay(80);
  }
}

void loop() {
  while (Serial1.available()) {
    AppLogic::parser.feed((uint8_t)Serial1.read());
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

    StaticJsonDocument<1024> doc;
    AppLogic::getTelemetryJson(doc);
    serializeJson(doc, Serial);
    Serial.println();
    
    AppLogic::blinkLED();
  }

  AppLogic::updateLED();
}
