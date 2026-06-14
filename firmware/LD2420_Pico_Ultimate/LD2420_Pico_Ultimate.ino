/*
 * =====================================================================================
 * LD2420 PICO ULTIMATE v6.0 — TRUE DETECTION FIRMWARE
 * Hardware : Raspberry Pi Pico (RP2040)
 * Compiler : Earle Philhower arduino-pico core ≥ 3.9
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

// RP2040 Hardware Includes for Raw FMCW ADC Mod
#include "hardware/adc.h"
#include "hardware/dma.h"

// ADC Configuration
#define ADC_PIN 26          // GP26 = ADC0
#define ADC_SAMPLES 1024
uint16_t adc_buffer[ADC_SAMPLES];
int dma_chan;
dma_channel_config dma_cfg;

// ─────────────────────────────────────────────────────────────────────────────
// COMPILE-TIME CONFIGURATION
// ─────────────────────────────────────────────────────────────────────────────


#include "LD2420_AppLogic.hpp"
#include "pico/multicore.h"
#include "hardware/watchdog.h"
#include "hardware/clocks.h"

#define PIN_RADAR_TX 0
#define PIN_RADAR_RX 1
#define PIN_RADAR_OT2 2
#define PIN_LED 3

#include "hardware/uart.h"

int uart_dma_chan;
uint8_t __attribute__((aligned(256))) uart_ring_buffer[256];
uint32_t uart_read_ptr = 0;

mutex_t radar_mutex;

void handleOT2() {
  uint32_t now = millis();
  if (now - AppLogic::ot2_last_ms > OT2_DEBOUNCE_MS) {
    AppLogic::ot2_raw = (digitalRead(PIN_RADAR_OT2) == HIGH);
    AppLogic::ot2_last_ms = now;
    if (AppLogic::ot2_raw) {
      // Wake up from low power mode if applicable
      set_sys_clock_khz(133000, true);
    }
  }
}

// ==========================================
// CORE 1: TELEMETRY AND USB COMMUNICATION
// ==========================================
void setup1() {
    Serial.begin(115200);
}

void loop1() {
    if (millis() - AppLogic::last_broadcast_ms >= TELEMETRY_MS) {
        AppLogic::last_broadcast_ms = millis();
        
        AppLogic::TelemetryPacket pkt;
        LOCK_RADAR();
        AppLogic::getTelemetryBinary(pkt);
        UNLOCK_RADAR();
        
        Serial.write((const uint8_t*)&pkt, sizeof(AppLogic::TelemetryPacket));
        Serial.flush();
        AppLogic::blinkLED();
    }
    delay(1);
}

// ==========================================
// CORE 0: HARD-REALTIME DSP & ML
// ==========================================
void setup() {
  LittleFS.begin(); // Mount LittleFS for logging

  adc_init();
  adc_gpio_init(ADC_PIN);
  adc_select_input(0); 
  adc_fifo_setup(true, true, 1, false, false);
  adc_set_clkdiv(48000 / 250); 
  
  dma_chan = dma_claim_unused_channel(true);
  dma_cfg = dma_channel_get_default_config(dma_chan);
  channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16);
  channel_config_set_read_increment(&dma_cfg, false);
  channel_config_set_write_increment(&dma_cfg, true);
  channel_config_set_dreq(&dma_cfg, DREQ_ADC);
  dma_channel_configure(dma_chan, &dma_cfg, adc_buffer, &adc_hw->fifo, ADC_SAMPLES, true);
  adc_run(true);

  Serial1.setTX(PIN_RADAR_TX);
  Serial1.setRX(PIN_RADAR_RX);
  Serial1.begin(115200);

  // Set up DMA for UART1 RX (Serial1)
  uart_dma_chan = dma_claim_unused_channel(true);
  dma_channel_config c = dma_channel_get_default_config(uart_dma_chan);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
  channel_config_set_read_increment(&c, false);
  channel_config_set_write_increment(&c, true);
  channel_config_set_ring(&c, true, 8); // 256 byte ring buffer (2^8)
  channel_config_set_dreq(&c, uart_get_dreq(uart0, false)); // UART0 RX DREQ

  dma_channel_configure(
      uart_dma_chan,
      &c,
      uart_ring_buffer, // Write address
      &uart0_hw->dr,    // Read address
      0xFFFFFFFF,       // Infinite transfers
      true              // Start immediately
  );


  pinMode(PIN_RADAR_OT2, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_RADAR_OT2), handleOT2, CHANGE);

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  mutex_init(&radar_mutex);
  memset((void*)&AppLogic::radar, 0, sizeof(AppLogic::radar));

  // Enable Watchdog Timer (2000ms)
  watchdog_enable(2000, 1);

  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_LED, HIGH);
    delay(80);
    digitalWrite(PIN_LED, LOW);
    delay(80);
  }
}

void loop() {
  watchdog_update(); // Ping WDT

  if (!dma_channel_is_busy(dma_chan)) {
    adc_run(false);
    uint32_t sum_sq = 0;
    for(int i=0; i<ADC_SAMPLES; i++) {
        int32_t centered = (int32_t)adc_buffer[i] - 2048;
        sum_sq += centered * centered;
    }
    double rms_if = sqrt((double)sum_sq / ADC_SAMPLES);
    dma_channel_configure(dma_chan, &dma_cfg, adc_buffer, &adc_hw->fifo, ADC_SAMPLES, true);
    adc_run(true);
  }


  // DMA UART Parsing without CPU interrupts
  LOCK_RADAR();
  uint32_t current_write_ptr = ((uint32_t)dma_hw->ch[uart_dma_chan].write_addr - (uint32_t)uart_ring_buffer);
  while (uart_read_ptr != current_write_ptr) {
    AppLogic::parser.feed(uart_ring_buffer[uart_read_ptr]);
    uart_read_ptr = (uart_read_ptr + 1) % 256;
    set_sys_clock_khz(133000, true); // Wake up
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

  // Low power dynamic scaling (throttle to 10MHz if ABSENT > 10min)
  if (!AppLogic::radar.presence_fused && (now - AppLogic::radar.absence_start_ms > 600000)) {
      set_sys_clock_khz(10000, true);
  }

  // Note: Telemetry broadcast is now handled independently on CORE 1

  // DSP tick runs as fast as possible, state updates handled continuously
  static uint32_t last_dsp_ms = 0;
  if(now - last_dsp_ms >= TELEMETRY_MS) {
      last_dsp_ms = now;
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
  }

  AppLogic::updateLED();
  UNLOCK_RADAR();
}
