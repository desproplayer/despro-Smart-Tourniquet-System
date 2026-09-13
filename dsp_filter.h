#pragma once
#include <Arduino.h>
#include "config.h"

// Filter Butterworth bandpass orde 5 (1-5 Hz @ fs=50Hz), Direct Form II Transposed,
// diimplementasikan sebagai 5 buah biquad (Second-Order Sections) berurutan.
//
// Koefisien dihitung dengan scipy.signal.butter(5, [1,5], btype='bandpass', fs=50, output='sos')
// lalu di-hardcode di sini (lihat Docs/perhitungan_butterworth.md dan
// Algorithms/butterworth_filter_design.py untuk derivasinya & validasi stabilitas/respons
// frekuensi).
//
// REVISI: settling time setelah reset filter.
// Mereset filter setiap window sampling baru menciptakan TRANSIEN (filter belum settle) di
// awal window, yang berisiko mengganggu akurasi peak detection tepat saat paling kritis.
// Group delay filter ini di frekuensi tengah passband (~3Hz) dihitung ~10.4 sampel
// (lihat Algorithms/butterworth_filter_design.py); dengan margin 2.5x, FILTER_SETTLE_SAMPLES
// = 27 sampel (~540ms @ 50Hz) dibuang dari peak detection setiap kali filter direset.
//
// Ini BELUM final -- masih perlu diuji dengan data phantom arm real untuk memastikan tidak
// menimbulkan false negative/positive pada deteksi LOP (lihat catatan di
// Docs/perhitungan_butterworth.md bagian 5). Alternatif Opsi A (filter berjalan kontinu,
// tidak pernah direset) juga tersedia lewat flag USE_CONTINUOUS_FILTER di bawah -- ganti
// jika hasil uji phantom arm menunjukkan pendekatan ini lebih baik.

#define FILTER_SETTLE_SAMPLES 27       // sampel dibuang setelah reset (~540ms @ 50Hz)
#define USE_CONTINUOUS_FILTER false     // true = filter tidak pernah direset antar window (Opsi A)

struct BiquadSection {
    float b0, b1, b2;
    float a1, a2;
    // state Direct Form II Transposed
    float z1 = 0.0f;
    float z2 = 0.0f;

    float process(float x) {
        float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void reset() {
        z1 = 0.0f;
        z2 = 0.0f;
    }
};

class ButterworthBPF {
public:
    ButterworthBPF() {
        // clang-format off
        sections[0] = {0.0004894361f, 0.0009788722f, 0.0004894361f, -1.3198965304f, 0.5281847481f};
        sections[1] = {1.0000000000f, 2.0000000000f, 1.0000000000f, -1.5276383342f, 0.5913983514f};
        sections[2] = {1.0000000000f, 0.0000000000f, -1.0000000000f, -1.4527179777f, 0.7819157637f};
        sections[3] = {1.0000000000f, -2.0000000000f, 1.0000000000f, -1.8105486220f, 0.8313586394f};
        sections[4] = {1.0000000000f, -2.0000000000f, 1.0000000000f, -1.9315415226f, 0.9474689283f};
        // clang-format on
    }

    float process(float x) {
        float y = x;
        for (auto &s : sections) {
            y = s.process(y);
        }
        return y;
    }

    // Hanya dipanggil kalau USE_CONTINUOUS_FILTER == false (Opsi B: reset + buang settling samples)
    void reset() {
        if (!USE_CONTINUOUS_FILTER) {
            for (auto &s : sections) s.reset();
        }
        // kalau USE_CONTINUOUS_FILTER == true, reset() sengaja jadi no-op -- filter
        // terus jalan dari state sebelumnya tanpa terputus antar window (Opsi A)
    }

private:
    BiquadSection sections[BUTTERWORTH_ORDER];
};

// Peak detection sederhana pada sliding window (dipakai per window sampling 3 detik / 150 sampel)
class PeakDetector {
public:
    // Menambahkan satu sampel hasil filter, mendeteksi local maxima dengan sliding window
    // lebar WINDOW_SAMPLES (default 0.5 detik @ 50Hz = 25 sampel), sesuai proposal Bab VI.C.
    static const int WINDOW_SAMPLES = 25;

    void reset() {
        count = 0;
        peakCount = 0;
        idx = 0;
        settleCounter = 0; // mulai hitung ulang sampel yang perlu dibuang untuk settling
    }

    // Menambahkan satu sampel HASIL FILTER. Sampel pertama sebanyak FILTER_SETTLE_SAMPLES
    // (setelah reset() dipanggil) diabaikan dari peak counting -- ini menangani transien
    // filter pasca-reset (lihat catatan revisi di atas), bukan cuma isu sliding window biasa.
    void addSample(float value) {
        if (!USE_CONTINUOUS_FILTER && settleCounter < FILTER_SETTLE_SAMPLES) {
            settleCounter++;
            return; // buang sampel ini, filter belum settle pasca-reset
        }

        buffer[idx % WINDOW_SAMPLES] = value;
        idx++;
        if (count < WINDOW_SAMPLES) count++;

        // deteksi local max di tengah window (butuh window penuh dulu)
        if (count == WINDOW_SAMPLES) {
            int midIdx = (idx - 1 - WINDOW_SAMPLES / 2) % WINDOW_SAMPLES;
            float midVal = buffer[(midIdx + WINDOW_SAMPLES) % WINDOW_SAMPLES];
            bool isPeak = true;
            for (int k = 0; k < WINDOW_SAMPLES; k++) {
                if (k == midIdx) continue;
                if (buffer[k] > midVal) {
                    isPeak = false;
                    break;
                }
            }
            // prominence threshold empiris -- BELUM FINAL, wajib dikalibrasi dari data
            // phantom arm (PIC: Grace/Biomedik) sebelum dipakai untuk klaim performa final
            if (isPeak && midVal > peakProminenceThreshold) {
                peakCount++;
            }
        }
    }

    int getPeakCount() const { return peakCount; }

    void setProminenceThreshold(float t) { peakProminenceThreshold = t; }

private:
    float buffer[WINDOW_SAMPLES] = {0};
    int idx = 0;
    int count = 0;
    int peakCount = 0;
    int settleCounter = 0;
    float peakProminenceThreshold = 0.05f; // mmHg, NILAI AWAL -- wajib dikalibrasi saat uji phantom arm
};
