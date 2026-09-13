# Perhitungan Filter Butterworth Bandpass Orde 5

## 1. Spesifikasi (sesuai proposal Bab II.B & VI.C)

- Orde: 5
- Band-pass: 1-5 Hz (mengisolasi pulsasi jantung 0.8-3 Hz dari noise)
- Sampling rate: 50 Hz
- Implementasi: Direct Form II Transposed (stabilitas numerik lebih baik untuk fixed/float embedded)

## 2. Cara hitung koefisien (Python/scipy, sesuai metode di proposal)

```python
from scipy import signal
sos = signal.butter(5, [1.0, 5.0], btype='bandpass', fs=50.0, output='sos')
```

`output='sos'` (Second-Order Sections) dipilih daripada `output='ba'` karena bandpass Butterworth
orde tinggi (disini jadi orde 10 karena band-pass = 2x orde lowpass prototype) numerically unstable
kalau direpresentasikan sebagai satu polinomial b/a orde tinggi langsung. SOS memecahnya jadi 5
biquad (filter orde 2) berurutan yang jauh lebih stabil untuk floating-point 32-bit di ESP32.

## 3. Hasil koefisien (di-hardcode di `dsp_filter.h`)

| Section | b0 | b1 | b2 | a1 | a2 |
|---|---|---|---|---|---|
| 0 | 0.0004894361 | 0.0009788722 | 0.0004894361 | -1.3198965304 | 0.5281847481 |
| 1 | 1.0 | 2.0 | 1.0 | -1.5276383342 | 0.5913983514 |
| 2 | 1.0 | 0.0 | -1.0 | -1.4527179777 | 0.7819157637 |
| 3 | 1.0 | -2.0 | 1.0 | -1.8105486220 | 0.8313586394 |
| 4 | 1.0 | -2.0 | 1.0 | -1.9315415226 | 0.9474689283 |

## 4. Validasi stabilitas & respons frekuensi

Pole magnitude tiap section (harus <1 untuk stabil):
0.727, 0.769, 0.884, 0.912, 0.973 — **semua stabil**.

Respons magnitude (dB):
| Freq (Hz) | Magnitude (dB) |
|---|---|
| 0.5 | -37.34 (diredam kuat, di luar band) |
| 1.0 | -3.01 (edge band bawah) |
| 2.0 | 0.00 (passband) |
| 3.0 | 0.00 (passband) |
| 5.0 | -3.01 (edge band atas) |
| 8.0 | -29.15 |
| 15.0 | -71.57 |
| 24.9 (Nyquist) | -239.90 |

Filter sudah tervalidasi: passband rata di 1-5 Hz, roll-off tajam di luar band, cocok untuk
mengisolasi sinyal oscillometric pulsasi arteri dari noise mekanik (getaran lapangan, switching
noise pompa BLDC).

## 5. Catatan implementasi

- `PeakDetector::peakProminenceThreshold` (default 0.05 mmHg) adalah nilai awal — **wajib
  dikalibrasi ulang** saat uji phantom arm (PIC: Grace/Biomedik), sesuai catatan proposal
  "prominence threshold empiris yang dikalibrasi pada phantom arm".
- Reset filter (`bpFilter.reset()`) wajib dipanggil setiap kali memulai window sampling baru,
  supaya state biquad dari step tekanan sebelumnya tidak mencemari deteksi peak di step berikutnya.
