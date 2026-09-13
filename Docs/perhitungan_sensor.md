# Perhitungan Transfer Function Sensor MPX5050DP

## 1. Rumus dasar (datasheet Motorola/NXP)

    Vout = VS x (0.018 x P_kPa + 0.04)

## 2. Konversi ke mmHg (1 kPa = 7.50062 mmHg), VS = 5.0V

    Vout = 0.012 x P_mmHg + 0.2

Sensitivitas efektif: **12 mV/mmHg**, offset **0.2 V** pada P = 0.

Rumus invers (dipakai di firmware, `pressure_sensor.h`):

    P_mmHg = (Vout - 0.2) / 0.012

## 3. Tabel validasi

| P (mmHg) | Vout (V) |
|---|---|
| 0   | 0.200 |
| 50  | 0.800 |
| 100 | 1.400 |
| 150 | 2.000 |
| 220 (target LOP lengan) | 2.840 |
| 280 (target LOP paha)   | 3.560 |
| 300 | 3.800 |
| 375 (batas maks sensor) | 4.700 |

## 4. Catatan penting

- **Saturasi sensor di 375 mmHg** — PDS proyek menargetkan hard limit 400 mmHg (spec A.1), tapi
  MPX5050DP secara fisik mentok di Vout = VS pada ~375 mmHg. Perlu didiskusikan dengan pembimbing:
  turunkan target ke <=375 mmHg, atau ganti sensor (mis. seri MPX5100, range lebih tinggi).
- **Akurasi sensor ±2.5% VFSS** = ±0.1125V = **±9.4 mmHg**, jauh dari target PDS ±2 mmHg (spec A.4).
  Solusi: kalibrasi multi-titik terhadap manometer referensi (bagian dari protokol validasi phantom
  arm, PIC: Grace/Biomedik) + kompensasi suhu di software.
- **ADS1115 wajib disupply 5V** (bukan 3.3V) karena Vout sensor bisa sampai 4.7V, melebihi batas
  mutlak input jika ADS1115 di-supply 3.3V. Jalur I2C tetap aman ke ESP32 (3.3V logic) karena
  SDA/SCL open-drain, pull-up ditarik ke 3.3V.
- Gain ADS1115 yang dipakai: `GAIN_TWOTHIRDS` (range ±6.144V) — resolusi ~0.0156 mmHg/bit, jauh
  lebih presisi dari kebutuhan (1 mmHg).
