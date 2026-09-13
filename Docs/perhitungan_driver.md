# Perhitungan Driver MOSFET & Daya/Baterai

## 1. Pemilihan MOSFET

IRF540N (disebut di proposal) adalah standard-level MOSFET, butuh VGS~10V untuk RDS(on) optimal.
ESP32-S3 GPIO cuma 3.3V -> MOSFET tidak fully-on -> disipasi panas tinggi.

**Solusi: ganti ke logic-level MOSFET, mis. IRLZ44N atau IRLZ34N** (fully-on di VGS 3.3-5V).
Alternatif kalau tetap mau pakai IRF540N: tambah IC gate driver (IR2104) atau pre-driver transistor NPN.

## 2. Perhitungan rangkaian gate (IRLZ44N)

- Gate resistor RG = 100-220 ohm (batas arus GPIO ESP32-S3 maks 40mA/pin)
- Pull-down gate-source RPD = 10k ohm (WAJIB, cegah MOSFET nyala sendiri saat GPIO high-Z di boot)
- Flyback diode 1N5819 (Schottky) paralel terbalik dengan tiap beban induktif (pompa & solenoid)
  -> WAJIB, mencegah voltage spike dari energi induktor saat switch-off

## 3. Disipasi daya MOSFET

RDS(on) IRLZ44N @ VGS=5V ~ 0.022 ohm
P_cond = I^2 x RDS(on) = (0.4A)^2 x 0.022 = 3.5 mW -> tidak perlu heatsink untuk skala arus ini.

## 4. Perhitungan daya total sistem

### Breakdown arus per komponen
| Komponen | Arus | Rail |
|---|---|---|
| ESP32-S3 aktif | 80 mA | 3.3V |
| Pompa BLDC (peak) | 400 mA | 12V |
| Solenoid (x1 aktif) | 200 mA | 12V |
| OLED | 20 mA | 3.3V |
| ADS1115 + sensor | ~5 mA | 5V |
| Buzzer aktif | ~30 mA | 3.3V |
| LED indikator | ~10 mA | 3.3V |

### Konversi ke sisi baterai (3.7V) via boost converter (eff. 93%) dan LDO (asumsi eff. 85%)

Mode INFLATING:
- Beban 12V (pompa): P = 12V x 0.4A = 4.8W -> P_bat = 4.8/0.93 = 5.16W -> I_bat = 5.16/3.7 = 1.39A
- Beban 3.3V (ESP32+OLED+sensor): P = 0.35W -> P_bat = 0.35/0.85 = 0.41W -> I_bat = 0.11A
- **Total I_bat saat INFLATING ~ 1.5A**

  Catatan: ini lebih tinggi dari klaim proposal "peak <700mA" karena proposal menghitung arus
  di sisi 12V langsung tanpa konversi efisiensi ke sisi baterai 3.7V. Perlu direvisi di laporan.

Mode HOLDING (solenoid NC tanpa daya = tertutup, MCU light sleep):
- I_bat ~ 5 mA -> P ~ 0.0185W

### Estimasi umur baterai (LiPo 2000mAh, 3.7V)

Skenario per siklus pakai (30 detik inflasi + 1 jam holding):
- Fase inflasi: 1.5A x (30/3600)h = 12.5 mAh
- Fase holding (1 jam): 5mA x 1h = 5 mAh
- Total per siklus ~ 17.5 mAh -> baterai 2000mAh sanggup >100 siklus (skenario dominan holding)

Worst case (re-inflasi kontinu terus-menerus, mis. karena kebocoran cuff):
- t_max = 2000mAh / 1500mA = 1.33 jam
- Masih memenuhi target PDS spec D.1 (minimum 2 jam operasi aktif)? MEPET -- perlu divalidasi
  di pengujian nyata, terutama karena kendala Week 3 (mini air pump lambat capai target tekanan
  -> makin sering re-inflate -> makin boros baterai).

## 5. Rekomendasi tindak lanjut
- Validasi arus real pakai multimeter/current clamp saat pompa jalan (jangan cuma andalkan
  datasheet nominal)
- Kalau hasil ukur mendekati worst-case, pertimbangkan baterai kapasitas lebih besar
  (mis. 3000-4000 mAh) atau optimasi PWM duty cycle pompa
