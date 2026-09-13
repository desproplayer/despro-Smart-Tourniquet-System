# Perhitungan Transfer Function Sensor MPX5050DP

**Status revisi:** dokumen ini sudah direview — lihat bagian "Isu terbuka" di bawah untuk poin
yang masih perlu keputusan desain atau validasi empiris sebelum dianggap final.

## 1. Rumus dasar (datasheet Motorola/NXP)

    Vout = VS x (0.018 x P_kPa + 0.04)

## 2. Konversi ke mmHg (1 kPa = 7.50062 mmHg), VS = 5.0V

    Vout = 0.012 x P_mmHg + 0.2

Sensitivitas efektif: **12 mV/mmHg**, offset **0.2 V** pada P = 0.
(Rumus ini sudah divalidasi ulang lewat `Algorithms/sensor_conversion.py` — selisih terhadap
rumus datasheet asli <0.0005V di seluruh rentang, dapat diabaikan.)

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
| 375 (batas rated/POP datasheet) | 4.700 |
| ~400 (titik saturasi elektris, Vout=VS) | ~5.000 |

## 4. Isu terbuka (WAJIB diputuskan/divalidasi sebelum implementasi final)

### 4.1 Rentang sensor 375 mmHg vs target hard limit 400 mmHg (PDS spec A.1) — BELUM DIPUTUSKAN

Koreksi dari catatan sebelumnya: sensor **tidak clipping elektris** sampai kira-kira 400 mmHg
(Vout baru menyentuh VS di titik itu, lihat `Algorithms/sensor_conversion.py`). Tapi **375 mmHg
adalah batas rentang operasi TERKALIBRASI (rated/POP)** menurut datasheet — akurasi dan
linearitas sensor di atas titik itu (375-400 mmHg) TIDAK dijamin pabrikan, meski secara sinyal
ADC masih bisa membaca angka.

Konflik requirement ini harus diputuskan tim, salah satu dari:
- **Opsi A:** turunkan hard limit software sistem menjadi ≤375 mmHg (revisi PDS spec A.1)
- **Opsi B:** tetap pakai target 400 mmHg, tapi validasi manual dengan manometer referensi
  di rentang 375-400 mmHg untuk memastikan linearitas masih dapat diterima secara empiris
- **Opsi C:** ganti sensor ke seri dengan rentang lebih tinggi (mis. MPX5100 series)

Selama belum diputuskan, requirement sensor masih berstatus **konflik/terbuka**.

### 4.2 Akurasi sensor ±9.4 mmHg vs target PDS ±2 mmHg (spec A.4) — PERLU KALIBRASI

Akurasi mentah sensor (±2.5% VFSS = ±0.1125V = ±9.4 mmHg) jauh dari target PDS ±2 mmHg.
**Wajib** kalibrasi multi-titik terhadap manometer referensi (bagian protokol validasi phantom
arm, PIC: Grace/Biomedik) + kompensasi suhu di software. Tanpa kalibrasi ini, sistem TIDAK
memenuhi spec A.4.

### 4.3 Level tegangan ADS1115 vs I2C ESP32 — PERLU REVISI DESAIN

Catatan sebelumnya ("ADS1115 wajib disupply 5V, jalur I2C tetap aman ke ESP32 3.3V") **kurang
tepat dan perlu direvisi**. Alasan: kalau ADS1115 disupply 5V, level HIGH pada SDA/SCL yang
dijamin oleh datasheet ADS1115 mengacu ke VDD-nya (5V) — ini bisa melebihi batas toleransi input
GPIO ESP32-S3 (3.3V logic), tergantung karakteristik pull-up dan implementasi open-drain yang
sebenarnya dipakai. Asumsi "aman karena open-drain" di catatan awal tidak cukup dijamin tanpa
verifikasi datasheet spesifik.

**Pilih salah satu (BELUM diputuskan, perlu didiskusikan dengan tim Elektro):**
- **Opsi A (lebih aman):** ADS1115 disupply 3.3V. Konsekuensinya, Vout sensor MPX5050DP (bisa
  sampai ~4.7-5V) harus **diturunkan dulu** pakai voltage divider/scaling circuit sebelum masuk
  ke ADS1115, supaya tidak melebihi 3.3V. Ini mengubah rumus konversi tekanan di firmware
  (perlu faktor skala tambahan) dan mengubah pilihan gain ADS1115.
- **Opsi B:** ADS1115 tetap disupply 5V (mempertahankan rentang penuh sensor tanpa scaling),
  tapi tambahkan **I2C level shifter** (mis. modul bidirectional logic level converter berbasis
  BSS138) di antara ADS1115 dan ESP32.

Sampai salah satu opsi ini diimplementasikan dan diverifikasi, wiring yang tertulis di
`pressure_sensor.h` dan panduan wiring sebelumnya dianggap **belum final**.

### 4.4 Gain ADS1115 (tidak berubah, masih valid)

Gain `GAIN_TWOTHIRDS` (range ±6.144V) — resolusi ~0.0156 mmHg/bit, jauh lebih presisi dari
kebutuhan (1 mmHg). Ini valid terlepas dari opsi mana yang dipilih di poin 4.3 (kalau Opsi A
dipilih dan ada scaling, gain bisa disesuaikan lagi supaya tetap memakai rentang ADC optimal).
