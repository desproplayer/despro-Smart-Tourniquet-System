#pragma once
#include <Arduino.h>
#include "config.h"

// Driver MOSFET untuk pompa BLDC (PWM) dan solenoid valve (ON/OFF)
// Rangkaian & perhitungan gate resistor, pull-down, flyback diode: lihat Docs/perhitungan_driver.md
//
// PENTING: gunakan logic-level MOSFET (mis. IRLZ44N), BUKAN IRF540N standar,
// karena GPIO ESP32-S3 cuma 3.3V dan IRF540N butuh VGS~10V untuk fully-on.

#define PUMP_PWM_CHANNEL   0
#define PUMP_PWM_FREQ      20000   // 20 kHz, sesuai proposal (di atas audible range)
#define PUMP_PWM_RES       8       // 8-bit resolution (duty 0-255)

class ActuatorDriver {
public:
    void begin() {
        ledcSetup(PUMP_PWM_CHANNEL, PUMP_PWM_FREQ, PUMP_PWM_RES);
        ledcAttachPin(PUMP_MOSFET_PIN, PUMP_PWM_CHANNEL);

        pinMode(SOLENOID_LOCK_PIN, OUTPUT);
        pinMode(SOLENOID_RELEASE_PIN, OUTPUT);

        // default state aman: pompa off, solenoid lock CLOSED (NC = tertutup tanpa daya),
        // solenoid release CLOSED (tidak melepas tekanan)
        setPumpDuty(0);
        digitalWrite(SOLENOID_LOCK_PIN, LOW);
        digitalWrite(SOLENOID_RELEASE_PIN, LOW);
    }

    // duty 0-255. Dipakai saat fase INFLATING.
    void setPumpDuty(uint8_t duty) {
        ledcWrite(PUMP_PWM_CHANNEL, duty);
    }

    void pumpOff() {
        setPumpDuty(0);
    }

    // Kunci tekanan (dipanggil saat LOP tercapai — fase AUTO-STOP)
    void lockPressure(bool locked) {
        digitalWrite(SOLENOID_LOCK_PIN, locked ? HIGH : LOW);
    }

    // Slow-release terkontrol untuk T-Conversion di IGD
    void releaseSlow(bool releasing) {
        digitalWrite(SOLENOID_RELEASE_PIN, releasing ? HIGH : LOW);
    }

    // Emergency: buka penuh (dipakai bersamaan dengan fail-safe lever manual)
    void emergencyDeflate() {
        pumpOff();
        lockPressure(false);
        releaseSlow(true);
    }
};
