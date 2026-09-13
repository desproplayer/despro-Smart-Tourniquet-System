#pragma once
#include <Adafruit_ADS1X15.h>
#include "config.h"

// Driver sensor tekanan MPX5050DP via ADS1115
// Rumus transfer function & alasan pemilihan gain: lihat Docs/perhitungan_sensor.md

class PressureSensor {
public:
    bool begin() {
        if (!ads.begin(ADS1115_ADDR)) {
            return false;
        }
        ads.setGain(GAIN_TWOTHIRDS);   // range +-6.144V, wajib karena Vout MPX5050DP bisa sampai 4.7V
        return true;
    }

    // Baca tekanan dalam mmHg dari channel ADC tertentu (default channel 0)
    float readMmHg(uint8_t channel = 0) {
        int16_t raw = ads.readADC_SingleEnded(channel);
        float vout = raw * ADS1115_LSB_VOLT;
        float pressure = (vout - MPX5050_OFFSET_V) / MPX5050_SENSITIVITY_V_PER_MMHG;
        return pressure;
    }

private:
    Adafruit_ADS1115 ads;
    static constexpr float ADS1115_LSB_VOLT = 0.0001875f; // 6.144V / 32768, untuk GAIN_TWOTHIRDS
};
