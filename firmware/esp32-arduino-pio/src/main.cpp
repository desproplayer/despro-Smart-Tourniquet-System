// Smart Tourniquet System — main.cpp
// Kelompok 19 — Desain Proyek Teknik Elektro, Komputer, Biomedik 2
//
// Implementasi state machine penuh sesuai proposal Bab V.B (Alur Kerja Sistem) &
// Bab VI.C (State Machine dan Logika UI/UX):
//
//   IDLE -> SELF_CHECK -> ARMED -> INFLATING -> SAMPLING -> LOP_DETECTED
//         -> HOLDING -> MONITORING -> ALARM -> SLOW_RELEASE -> DEFLATED
//
// CATATAN: versi ini pakai loop() sederhana (bukan FreeRTOS multi-task penuh seperti
// disebut di proposal) supaya gampang di-debug dulu di hardware nyata. Setelah logic
// tervalidasi, pecah masing-masing tanggung jawab (Sensor Task, DSP Task, Control Task,
// UI Task, BLE Task) jadi FreeRTOS task terpisah sesuai Bab VI.C.

#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "pressure_sensor.h"
#include "dsp_filter.h"
#include "pid_controller.h"
#include "driver_mosfet.h"
#include "ui_driver.h"
#include "state_machine.h"

// ---------- Objek global ----------
PressureSensor pressureSensor;
ButterworthBPF bpFilter;
PeakDetector peakDetector;
PIDController pidController;
ActuatorDriver actuator;
UIDriver ui;

TourniquetState currentState = TourniquetState::IDLE;

// ---------- Parameter runtime ----------
float lopTargetMmHg = 0.0f;          // ditentukan setelah LOP terdeteksi
float currentSetpointMmHg = 180.0f;  // profil default awal, lengan atas (proposal V.B Fase 1)
bool isUpperLimb = true;             // true = lengan, false = paha (nanti dari selector fisik/BLE)

unsigned long stateEnteredMs = 0;
unsigned long holdingStartMs = 0;
unsigned long lastMonitorCheckMs = 0;
unsigned long lastUsageAlarmMs = 0;

// buffer sampling window (3 detik @ 50Hz = 150 sampel)
const int SAMPLES_PER_WINDOW = ADC_SAMPLE_RATE_HZ * SAMPLING_WINDOW_SEC;
int sampleCounter = 0;
unsigned long lastSampleMs = 0;
const unsigned long SAMPLE_INTERVAL_MS = 1000 / ADC_SAMPLE_RATE_HZ;

float lastKnownPressure = 0.0f;

// ---------- Helper: transisi state ----------
void transitionTo(TourniquetState newState) {
    Serial.printf("[STATE] %s -> %s\n", stateToString(currentState), stateToString(newState));
    currentState = newState;
    stateEnteredMs = millis();
}

// ---------- Setup ----------
void setup() {
    Serial.begin(115200);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    if (!pressureSensor.begin()) {
        Serial.println("ERROR: ADS1115 tidak terdeteksi.");
    }
    if (!ui.begin()) {
        Serial.println("ERROR: OLED tidak terdeteksi.");
    }
    actuator.begin(); // default aman: pompa off, semua solenoid closed

    pinMode(BTN_START_PIN, INPUT_PULLUP);
    pinMode(BTN_RELEASE_PIN, INPUT_PULLUP);

    transitionTo(TourniquetState::IDLE);
}

// ---------- Fase-fase state machine ----------

void handleIdle() {
    ui.showMessage("Smart Tourniquet", "Tekan POWER");
    // transisi ke SELF_CHECK terjadi otomatis begitu device menyala (asumsi power-on = start)
    transitionTo(TourniquetState::SELF_CHECK);
}

void handleSelfCheck() {
    ui.showSelfCheck();
    delay(2000); // sesuai proposal V.B Fase 0: self-check 2 detik
    ui.showMessage("SIAP", "Tekan MULAI");
    transitionTo(TourniquetState::ARMED);
}

void handleArmed() {
    // tunggu tombol START ditekan (active LOW karena INPUT_PULLUP)
    if (digitalRead(BTN_START_PIN) == LOW) {
        delay(50); // debounce sederhana
        if (digitalRead(BTN_START_PIN) == LOW) {
            currentSetpointMmHg = isUpperLimb ? 180.0f : 250.0f; // profil default V.B Fase 1
            bpFilter.reset();
            peakDetector.reset();
            pidController.reset();
            sampleCounter = 0;
            transitionTo(TourniquetState::INFLATING);
        }
    }
}

void handleInflating() {
    // PID mengejar currentSetpointMmHg secara bertahap (increment 15-30 mmHg per step,
    // sesuai proposal). Implementasi disederhanakan: PID langsung mengejar currentSetpointMmHg,
    // increment dilakukan setelah window sampling menyatakan "masih ada aliran darah".
    float pressure = pressureSensor.readMmHg();
    lastKnownPressure = pressure;

    if (pressure >= HARD_PRESSURE_LIMIT_MMHG) {
        // safety hard limit — jangan pernah lewati batas fisik sensor / batas keselamatan
        actuator.pumpOff();
        ui.showMessage("!! OVER LIMIT !!", "Cek sistem");
        ui.setStatusCritical();
        return; // tetap di state ini, tidak lanjut sampai direset manual
    }

    uint8_t duty = pidController.compute(currentSetpointMmHg, pressure);
    actuator.setPumpDuty(duty);

    ui.showPressureStatus(pressure, currentState, (millis() - stateEnteredMs) / 1000);

    // pompa berhenti sebentar di antara step untuk sampling (proposal V.B Fase 1: 3 detik)
    // -> disederhanakan: begitu mendekati setpoint (dalam toleransi kecil), masuk SAMPLING
    if (fabs(pressure - currentSetpointMmHg) < 3.0f) {
        actuator.pumpOff();
        bpFilter.reset();
        peakDetector.reset();
        sampleCounter = 0;
        lastSampleMs = millis();
        transitionTo(TourniquetState::SAMPLING);
    }
}

void handleSampling() {
    // Fase 2: jendela sampling 3 detik, 50Hz -> 150 sampel (proposal Bab V.B & VI.C)
    if (millis() - lastSampleMs >= SAMPLE_INTERVAL_MS) {
        lastSampleMs = millis();

        float pressure = pressureSensor.readMmHg();
        lastKnownPressure = pressure;

        float filtered = bpFilter.process(pressure);
        peakDetector.addSample(filtered);
        sampleCounter++;

        ui.showPressureStatus(pressure, currentState, (millis() - stateEnteredMs) / 1000);
    }

    if (sampleCounter >= SAMPLES_PER_WINDOW) {
        int peaks = peakDetector.getPeakCount();
        Serial.printf("[SAMPLING] Peaks terdeteksi: %d (threshold: %d)\n",
                      peaks, PEAK_THRESHOLD_MIN_COUNT);

        if (peaks > PEAK_THRESHOLD_MIN_COUNT) {
            // masih ada aliran darah -> naikkan tekanan, kembali INFLATING
            currentSetpointMmHg += 20.0f; // increment tengah rentang 15-30 mmHg proposal

            float maxTarget = isUpperLimb ? LOP_TARGET_MAX_UPPER_LIMB : LOP_TARGET_MAX_LOWER_LIMB;
            if (currentSetpointMmHg > maxTarget) {
                currentSetpointMmHg = maxTarget;
            }
            transitionTo(TourniquetState::INFLATING);
        } else {
            // oklusi tercapai (peaks <= threshold) -> LOP tercapai
            lopTargetMmHg = lastKnownPressure;
            transitionTo(TourniquetState::LOP_DETECTED);
        }
    }
}

void handleLopDetected() {
    actuator.pumpOff();
    actuator.lockPressure(true); // Fase 3: AUTO-STOP + kunci solenoid
    ui.showMessage("OKLUSI TERCAPAI", "");
    ui.buzzerBeepShort();
    ui.setStatusOK();
    Serial.printf("[LOP] Tekanan LOP tercapai: %.1f mmHg\n", lopTargetMmHg);

    holdingStartMs = millis();
    lastMonitorCheckMs = millis();
    lastUsageAlarmMs = millis();
    transitionTo(TourniquetState::HOLDING);
}

void handleHolding() {
    float pressure = pressureSensor.readMmHg();
    lastKnownPressure = pressure;

    unsigned long elapsedSec = (millis() - holdingStartMs) / 1000;
    ui.showPressureStatus(pressure, currentState, elapsedSec);

    // cek tombol RELEASE (T-Conversion manual oleh tenaga medis)
    if (digitalRead(BTN_RELEASE_PIN) == LOW) {
        delay(50);
        if (digitalRead(BTN_RELEASE_PIN) == LOW) {
            transitionTo(TourniquetState::SLOW_RELEASE);
            return;
        }
    }

    // masuk mode monitoring aktif setiap MONITOR_INTERVAL_SEC (Fase 4)
    if (millis() - lastMonitorCheckMs >= (unsigned long)MONITOR_INTERVAL_SEC * 1000) {
        lastMonitorCheckMs = millis();
        transitionTo(TourniquetState::MONITORING);
    }
}

void handleMonitoring() {
    float pressure = pressureSensor.readMmHg();
    lastKnownPressure = pressure;

    float drop = lopTargetMmHg - pressure;

    unsigned long elapsedSec = (millis() - holdingStartMs) / 1000;
    ui.showPressureStatus(pressure, currentState, elapsedSec);

    // alarm penggunaan berkala (Fase 4c: tiap 30 menit)
    if (millis() - lastUsageAlarmMs >= (unsigned long)USAGE_ALARM_INTERVAL_MIN * 60000UL) {
        lastUsageAlarmMs = millis();
        ui.buzzerBeepShort();
        ui.showMessage("EVALUASI MEDIS", "diperlukan segera");
        delay(1500);
    }

    if (drop > PRESSURE_DROP_LEAK_MMHG) {
        // penurunan sangat cepat -> indikasi kebocoran (Fase 4b)
        transitionTo(TourniquetState::ALARM);
        return;
    }

    if (drop > PRESSURE_DROP_REINFLATE_MMHG) {
        // pelonggaran -> reinflasi otomatis (Fase 4a)
        ui.showMessage("SEDANG", "DIKENCANGKAN ULANG");
        ui.buzzerBeepShort();
        actuator.lockPressure(false);
        currentSetpointMmHg = lopTargetMmHg;
        pidController.reset();
        transitionTo(TourniquetState::INFLATING);
        return;
    }

    // tekanan masih baik -> kembali HOLDING
    transitionTo(TourniquetState::HOLDING);
}

void handleAlarm() {
    ui.showMessage("!! CUFF BOCOR !!", "KENCANGKAN STRAP");
    ui.buzzerAlarmContinuous();
    ui.blinkCriticalLed();

    // operator harus menekan RELEASE atau START untuk keluar dari ALARM setelah strap dibetulkan
    if (digitalRead(BTN_START_PIN) == LOW) {
        delay(50);
        if (digitalRead(BTN_START_PIN) == LOW) {
            ui.buzzerOff();
            currentSetpointMmHg = lopTargetMmHg;
            pidController.reset();
            transitionTo(TourniquetState::INFLATING);
        }
    }
    if (digitalRead(BTN_RELEASE_PIN) == LOW) {
        delay(50);
        if (digitalRead(BTN_RELEASE_PIN) == LOW) {
            ui.buzzerOff();
            transitionTo(TourniquetState::SLOW_RELEASE);
        }
    }
}

void handleSlowRelease() {
    // Fase 5: T-Conversion — slow release 20 mmHg/menit (proposal V.B & PDS spec A.6)
    static unsigned long lastStepMs = 0;
    const unsigned long stepIntervalMs = 1000; // update tiap 1 detik
    const float releasePerSecond = SLOW_RELEASE_RATE_MMHG_PER_MIN / 60.0f;

    ui.showMessage("T-CONVERSION", "Slow release...");
    actuator.releaseSlow(true);

    if (millis() - lastStepMs >= stepIntervalMs) {
        lastStepMs = millis();
        float pressure = pressureSensor.readMmHg();
        lastKnownPressure = pressure;

        Serial.printf("[SLOW_RELEASE] Tekanan: %.1f mmHg (target turun %.2f mmHg/s)\n",
                      pressure, releasePerSecond);

        if (pressure <= 10.0f) { // dianggap deflate sempurna
            actuator.releaseSlow(false);
            transitionTo(TourniquetState::DEFLATED);
        }
    }
}

void handleDeflated() {
    ui.showMessage("CUFF DEFLATE", "SEMPURNA");
    actuator.emergencyDeflate(); // pastikan semua aktuator dalam state aman
    ui.setStatusOK();
    // sistem berhenti di sini — perlu restart manual (Power OFF/ON) untuk siklus baru
}

// ---------- Main loop ----------
void loop() {
    switch (currentState) {
        case TourniquetState::IDLE:         handleIdle(); break;
        case TourniquetState::SELF_CHECK:   handleSelfCheck(); break;
        case TourniquetState::ARMED:        handleArmed(); break;
        case TourniquetState::INFLATING:    handleInflating(); break;
        case TourniquetState::SAMPLING:     handleSampling(); break;
        case TourniquetState::LOP_DETECTED: handleLopDetected(); break;
        case TourniquetState::HOLDING:      handleHolding(); break;
        case TourniquetState::MONITORING:   handleMonitoring(); break;
        case TourniquetState::ALARM:        handleAlarm(); break;
        case TourniquetState::SLOW_RELEASE: handleSlowRelease(); break;
        case TourniquetState::DEFLATED:     handleDeflated(); break;
    }
    delay(20); // ~50Hz loop rate dasar; sampling presisi tetap dikontrol oleh SAMPLE_INTERVAL_MS
}
