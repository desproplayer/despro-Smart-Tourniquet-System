#pragma once

// ============ Smart Tourniquet System — Pin & Parameter Config ============
// Kelompok 19 — Desain Proyek Teknik Elektro, Komputer, Biomedik 2

// --- I2C (ADS1115 + OLED) ---
#define I2C_SDA_PIN        21
#define I2C_SCL_PIN        22
#define ADS1115_ADDR       0x48
#define OLED_ADDR          0x3C
#define OLED_WIDTH         128
#define OLED_HEIGHT        64

// --- Aktuator (lihat driver_mosfet.h) ---
#define PUMP_MOSFET_PIN         25   // PWM ke gate MOSFET driver pompa BLDC
#define SOLENOID_LOCK_PIN       26   // Solenoid valve NC — kunci tekanan
#define SOLENOID_RELEASE_PIN    27   // Solenoid valve slow-release — T-Conversion

// --- UI ---
#define BTN_START_PIN      32
#define BTN_RELEASE_PIN    33
#define BUZZER_PIN         14
#define LED_GREEN_PIN      12
#define LED_YELLOW_PIN     13
#define LED_RED_PIN        15

// --- Kalibrasi Sensor MPX5050DP (lihat perhitungan transfer function) ---
// Vout = 0.012 * P_mmHg + 0.2   (VS = 5.0V)
// P_mmHg = (Vout - OFFSET_V) / SENSITIVITY_V_PER_MMHG
#define MPX5050_SENSITIVITY_V_PER_MMHG   0.012f
#define MPX5050_OFFSET_V                 0.2f
#define MPX5050_MAX_MMHG                 375.0f   // batas fisik sensor — JANGAN set target > ini

// --- Parameter Klinis (PDS — lihat Lampiran proposal) ---
#define LOP_TARGET_MIN_UPPER_LIMB   150
#define LOP_TARGET_MAX_UPPER_LIMB   250
#define LOP_TARGET_MIN_LOWER_LIMB   200
#define LOP_TARGET_MAX_LOWER_LIMB   320
#define HARD_PRESSURE_LIMIT_MMHG    375   // disesuaikan dari 400 -> batas fisik sensor (lihat catatan README Docs)

// --- DSP ---
#define ADC_SAMPLE_RATE_HZ       50
#define SAMPLING_WINDOW_SEC      3
#define BUTTERWORTH_ORDER        5
#define BPF_LOW_HZ               1.0f
#define BPF_HIGH_HZ              5.0f
#define PEAK_THRESHOLD_MIN_COUNT 3     // jumlah puncak per window -> LOP tercapai jika <= ini

// --- Monitoring Aktif ---
#define MONITOR_INTERVAL_SEC        15
#define PRESSURE_DROP_REINFLATE_MMHG   15
#define PRESSURE_DROP_LEAK_MMHG        30
#define LEAK_DETECTION_WINDOW_SEC      5
#define USAGE_ALARM_INTERVAL_MIN       30

// --- T-Conversion ---
#define SLOW_RELEASE_RATE_MMHG_PER_MIN   20
