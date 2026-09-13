#pragma once
#include <Arduino.h>
#include "config.h"

// Driver MOSFET untuk pompa BLDC (PWM) dan solenoid valve (ON/OFF)
// Rangkaian & perhitungan gate resistor, pull-down, flyback diode: lihat Docs/perhitungan_driver.md
//
// REVISI PENTING -- pemilihan MOSFET:
// Rekomendasi sebelumnya ("pakai IRLZ44N karena logic-level, fully-on di 3.3V") KURANG TEPAT
// tanpa syarat. IRLZ44N memang tergolong logic-level MOSFET, tapi nilai RDS(on) yang dipakai
// di perhitungan daya (Docs/perhitungan_driver.md) adalah RDS(on) @ VGS=5V dari datasheet --
// BUKAN kondisi yang dijamin pabrikan pada VGS=3.3V (tegangan output GPIO ESP32-S3).
//
// Kode driver ini (ledcWrite/digitalWrite) BEKERJA SAMA terlepas dari MOSFET spesifik yang
// dipasang di hardware -- tapi part fisik yang dipasang harus dipilih dari salah satu:
//   Opsi A (direkomendasikan): MOSFET dengan RDS(on) TERJAMIN datasheet pada VGS=2.5-3.3V,
//           mis. IRLML2502, AO3400, atau seri logic-level modern lain yang eksplisit
//           mencantumkan RDS(on) di VGS rendah pada tabel elektrikalnya.
//   Opsi B: tetap pakai IRLZ44N untuk prototype, TAPI wajib validasi empiris (ukur suhu &
//           arus real saat beroperasi di VGS=3.3V) sebelum dianggap desain final.
//   Opsi C: tambah pre-driver (transistor NPN kecil / IC gate driver IR2104) untuk menaikkan
//           tegangan gate ke 5-10V, sehingga IRF540N/IRLZ44N bisa beroperasi di RDS(on) optimal.
// Keputusan final BELUM diambil -- lihat Docs/perhitungan_driver.md bagian 1 untuk detail.

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
