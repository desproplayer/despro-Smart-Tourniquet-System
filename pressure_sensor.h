#pragma once
#include <Adafruit_ADS1X15.h>
#include "config.h"

// Driver sensor tekanan MPX5050DP via ADS1115
// Rumus transfer function: lihat Docs/perhitungan_sensor.md dan Algorithms/sensor_conversion.py
//
// REVISI PENTING -- keputusan level tegangan ADS1115 vs I2C ESP32-S3:
// Versi sebelumnya menyupply ADS1115 dari 5V dengan asumsi jalur I2C "aman" ke ESP32 (3.3V
// logic) karena open-drain. Asumsi itu tidak cukup dijamin tanpa verifikasi datasheet spesifik
// -- level HIGH yang dikeluarkan ADS1115 mengacu ke VDD-nya (5V), berisiko melebihi batas
// toleransi GPIO ESP32-S3.
//
// Implementasi di file ini memakai OPSI A (lebih aman): ADS1115 disupply 3.3V, dan tegangan
// output MPX5050DP (bisa sampai ~4.7-5V) diturunkan dulu pakai VOLTAGE DIVIDER sebelum masuk
// ke ADS1115. Rangkaian divider yang dipakai (nilai resistor E12 standar):
//
//   MPX5050DP Vout ---[R1 = 2.2k]---+---[R2 = 3.3k]--- GND
//                                    |
//                                 ke AIN0 ADS1115
//
//   Rasio pembagi = R2/(R1+R2) = 3.3/5.5 = 0.6
//   Vadc_max = 5.0V x 0.6 = 3.0V  -> aman di bawah batas 3.3V ADS1115 (ada margin 0.3V)
//
// Kalau tim memutuskan pakai OPSI B (ADS1115 tetap 5V + I2C level shifter) sebagai gantinya,
// set DIVIDER_RATIO = 1.0 di bawah (tidak ada pembagi tegangan) dan pastikan level shifter
// bidirectional (mis. modul BSS138) terpasang di jalur SDA/SCL sebelum ke ESP32.

#define DIVIDER_RATIO 0.6f   // ganti ke 1.0f kalau pakai Opsi B (level shifter, tanpa divider)

class PressureSensor {
public:
    bool begin() {
        if (!ads.begin(ADS1115_ADDR)) {
            return false;
        }
        // GAIN_ONE (range +-4.096V) cukup untuk Vadc maks ~3.0V hasil divider, dengan
        // resolusi lebih baik (0.125 mV/bit) dibanding GAIN_TWOTHIRDS yang dipakai versi
        // sebelumnya (waktu itu asumsinya tanpa divider, langsung baca sampai 4.7V).
        ads.setGain(GAIN_ONE);
        return true;
    }

    // Baca tekanan dalam mmHg dari channel ADC tertentu (default channel 0)
    float readMmHg(uint8_t channel = 0) {
        int16_t raw = ads.readADC_SingleEnded(channel);
        float vadc = raw * ADS1115_LSB_VOLT;

        // kompensasi voltage divider -> dapatkan balik Vout ASLI sensor sebelum dibagi
        float voutSensor = vadc / DIVIDER_RATIO;

        float pressure = (voutSensor - MPX5050_OFFSET_V) / MPX5050_SENSITIVITY_V_PER_MMHG;
        return pressure;
    }

private:
    Adafruit_ADS1115 ads;
    static constexpr float ADS1115_LSB_VOLT = 0.125e-3f; // 4.096V / 32768, untuk GAIN_ONE
};
