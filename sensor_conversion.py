"""
Smart Tourniquet System — Perhitungan Konversi Sensor MPX5050DP
Kelompok 19 — Desain Proyek Teknik Elektro, Komputer, Biomedik 2

Menghitung transfer function sensor tekanan MPX5050DP (Vout <-> mmHg),
memvalidasi terhadap datasheet, dan mengecek isu saturasi sensor vs target PDS.

Jalankan: python3 sensor_conversion.py
"""

VS = 5.0  # tegangan suplai sensor (Volt)
KPA_TO_MMHG = 7.50062

# --- Rumus dasar datasheet Motorola/NXP ---
# Vout = VS * (0.018 * P_kPa + 0.04)


def pressure_kpa_to_vout(p_kpa: float, vs: float = VS) -> float:
    return vs * (0.018 * p_kpa + 0.04)


def vout_to_pressure_kpa(vout: float, vs: float = VS) -> float:
    return (vout / vs - 0.04) / 0.018


def pressure_mmhg_to_vout(p_mmhg: float, vs: float = VS) -> float:
    p_kpa = p_mmhg / KPA_TO_MMHG
    return pressure_kpa_to_vout(p_kpa, vs)


def vout_to_pressure_mmhg(vout: float, vs: float = VS) -> float:
    p_kpa = vout_to_pressure_kpa(vout, vs)
    return p_kpa * KPA_TO_MMHG


# --- Rumus linear tersederhanakan (dipakai di firmware) ---
# Vout = 0.012 * P_mmHg + 0.2   (hasil substitusi VS=5V, konversi kPa->mmHg)
SENSITIVITY_V_PER_MMHG = 0.012
OFFSET_V = 0.2


def pressure_mmhg_to_vout_linear(p_mmhg: float) -> float:
    return SENSITIVITY_V_PER_MMHG * p_mmhg + OFFSET_V


def vout_to_pressure_mmhg_linear(vout: float) -> float:
    return (vout - OFFSET_V) / SENSITIVITY_V_PER_MMHG


if __name__ == "__main__":
    print("=" * 60)
    print("VALIDASI RUMUS: datasheet (kPa) vs linear-simplified (mmHg)")
    print("=" * 60)
    test_points_mmhg = [0, 50, 100, 150, 220, 280, 300, 375, 400]

    print(f"{'P (mmHg)':>10} | {'Vout datasheet':>15} | {'Vout linear':>13} | {'selisih':>8}")
    print("-" * 60)
    for p in test_points_mmhg:
        v_datasheet = pressure_mmhg_to_vout(p)
        v_linear = pressure_mmhg_to_vout_linear(p)
        diff = abs(v_datasheet - v_linear)
        print(f"{p:>10} | {v_datasheet:>15.4f} | {v_linear:>13.4f} | {diff:>8.5f}")

    print()
    print("=" * 60)
    print("CEK RENTANG SENSOR vs TARGET PDS (400 mmHg)")
    print("=" * 60)
    RATED_RANGE_MMHG = 375.0  # POP spec datasheet: 0-50 kPa = 0-375.03 mmHg
    p_electrical_saturation = vout_to_pressure_mmhg_linear(VS)  # Vout = VS -> ADC mentok
    print(f"Rentang OPERASI TERKALIBRASI (rated, POP spec datasheet): 0 - {RATED_RANGE_MMHG:.0f} mmHg")
    print(f"Titik SATURASI ELEKTRIS (Vout = VS, ADC benar-benar mentok): ~{p_electrical_saturation:.1f} mmHg")
    print(f"Target hard limit PDS: 400 mmHg")
    print()
    print("Koreksi penting (lihat catatan revisi di bawah):")
    print(f"  - Sensor TIDAK mengalami clipping elektris sampai ~{p_electrical_saturation:.0f} mmHg,")
    print(f"    jadi secara sinyal MASIH bisa membaca sampai target 400 mmHg.")
    print(f"  - TAPI akurasi & linearitas sensor cuma DIJAMIN pabrikan sampai "
          f"{RATED_RANGE_MMHG:.0f} mmHg (rentang rated/POP di datasheet).")
    print(f"    Antara {RATED_RANGE_MMHG:.0f}-400 mmHg, pembacaan masih keluar tapi TIDAK "
          f"bersertifikat akurat -> wajib divalidasi manual dengan manometer referensi")
    print(f"    kalau target hard limit 400 mmHg tetap dipertahankan.")

    print()
    print("=" * 60)
    print("CEK AKURASI SENSOR vs TARGET PDS (spec A.4: +/- 2 mmHg)")
    print("=" * 60)
    VFSS = 4.5  # Full Scale Span (V), dari datasheet
    accuracy_percent = 0.025  # 2.5% VFSS (datasheet)
    error_v = accuracy_percent * VFSS
    error_mmhg = error_v / SENSITIVITY_V_PER_MMHG
    print(f"Akurasi sensor (datasheet): +/-{accuracy_percent*100:.1f}% VFSS "
          f"= +/-{error_v:.4f} V = +/-{error_mmhg:.2f} mmHg")
    print(f"Target PDS spec A.4: +/-2 mmHg")
    if error_mmhg > 2.0:
        print(f"** GAP: error sensor mentah ({error_mmhg:.2f} mmHg) jauh melebihi target "
              f"({2.0} mmHg). Wajib kalibrasi multi-titik terhadap manometer referensi "
              f"+ kompensasi suhu di software. **")
    else:
        print("Akurasi sensor mentah sudah memenuhi target tanpa kalibrasi tambahan.")

    print()
    print("=" * 60)
    print("REVISI: LEVEL TEGANGAN ADS1115 vs I2C ESP32-S3")
    print("=" * 60)
    DIVIDER_RATIO = 0.6  # R2/(R1+R2) dengan R1=2.2k, R2=3.3k -- sesuai pressure_sensor.h
    ADS1115_SUPPLY_V = 3.3
    vout_max_sensor = pressure_mmhg_to_vout_linear(400)
    vadc_max_with_divider = vout_max_sensor * DIVIDER_RATIO
    print(f"Opsi A (dipakai di firmware pressure_sensor.h): ADS1115 disupply {ADS1115_SUPPLY_V}V,")
    print(f"Vout sensor diturunkan dulu pakai voltage divider (rasio {DIVIDER_RATIO}).")
    print(f"Vout sensor maksimum (di 400 mmHg): {vout_max_sensor:.3f} V")
    print(f"Vadc setelah divider: {vadc_max_with_divider:.3f} V "
          f"-> {'AMAN' if vadc_max_with_divider < ADS1115_SUPPLY_V else 'MASIH MELEBIHI BATAS!'} "
          f"(batas {ADS1115_SUPPLY_V}V, ada margin {ADS1115_SUPPLY_V - vadc_max_with_divider:.2f}V)")
    print()
    print("Opsi B (alternatif): ADS1115 tetap 5V + I2C level shifter (BSS138) di jalur SDA/SCL.")
    print("Kalau Opsi B dipilih, set DIVIDER_RATIO = 1.0 di pressure_sensor.h (tanpa pembagi tegangan).")

    print()
    print("=" * 60)
    print("KONFIGURASI ADS1115 (gain, resolusi) -- Opsi A dengan divider")
    print("=" * 60)
    ads_range_v = 4.096  # GAIN_ONE, sesuai pressure_sensor.h (Vadc maks ~3.0V dengan divider)
    ads_resolution_bits = 16
    ads_lsb_v = ads_range_v / (2 ** (ads_resolution_bits - 1))
    # LSB dalam mmHg harus dibagi juga dengan DIVIDER_RATIO karena kompensasi balik ke Vout asli
    ads_lsb_mmhg = (ads_lsb_v / DIVIDER_RATIO) / SENSITIVITY_V_PER_MMHG
    print(f"Gain dipakai: GAIN_ONE (range +/-{ads_range_v} V) -- cukup untuk Vadc maks ~3.0V")
    print(f"hasil divider, dengan resolusi lebih baik dibanding GAIN_TWOTHIRDS versi sebelumnya.")
    print(f"Resolusi ADC (di sisi Vadc): {ads_lsb_v*1000:.4f} mV/bit")
    print(f"Resolusi setelah dikompensasi divider (di sisi tekanan): {ads_lsb_mmhg:.4f} mmHg/bit")
    print(f"Masih jauh lebih presisi dari kebutuhan minimum (1 mmHg) -> resolusi bukan bottleneck.")
