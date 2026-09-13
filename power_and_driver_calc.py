"""
Smart Tourniquet System — Perhitungan Driver MOSFET & Daya/Baterai
Kelompok 19 — Desain Proyek Teknik Elektro, Komputer, Biomedik 2

Menghitung parameter gate MOSFET (gate resistor, disipasi daya),
serta estimasi konsumsi daya total sistem & umur baterai LiPo.

Jalankan: python3 power_and_driver_calc.py
"""

# ============================================================
# BAGIAN 1: Perhitungan Driver MOSFET (IRLZ44N, logic-level)
# ============================================================

def mosfet_gate_current(qg_nc: float, t_rise_us: float) -> float:
    """Arus gate rata-rata dibutuhkan untuk switching dalam waktu t_rise (mA)."""
    qg_c = qg_nc * 1e-9
    t_rise_s = t_rise_us * 1e-6
    ig_a = qg_c / t_rise_s
    return ig_a * 1000  # ke mA


def mosfet_conduction_loss(current_a: float, rds_on_ohm: float) -> float:
    """Disipasi daya konduksi P = I^2 * Rds(on), dalam Watt."""
    return (current_a ** 2) * rds_on_ohm


# ============================================================
# BAGIAN 2: Perhitungan Daya Total Sistem
# ============================================================

class LoadSpec:
    def __init__(self, name, voltage, current_a, rail):
        self.name = name
        self.voltage = voltage
        self.current_a = current_a
        self.rail = rail  # "3.3V", "5V", atau "12V"

    def power_w(self):
        return self.voltage * self.current_a


# Daftar beban saat mode INFLATING (semua aktif bersamaan)
#
# CATATAN PENTING (perlu klarifikasi tim Elektro/Hardware):
# Apakah solenoid_lock harus ENERGIZED (butuh daya) selama fase INFLATING supaya
# jalur udara pompa->bladder terbuka (jika solenoid itu NC = closed tanpa daya,
# menutup jalur), atau solenoid_lock hanya dipakai SETELAH LOP tercapai untuk
# menyegel tekanan (dan selama inflating jalurnya terbuka lewat check valve tanpa
# perlu daya)? Ini menentukan apakah baris "Solenoid (1 aktif)" di bawah relevan
# untuk mode INFLATING atau tidak -- dan berdampak besar ke hasil worst-case
# battery life (lihat perbandingan di README/perhitungan_driver.md).
# Firmware saat ini (main.cpp) TIDAK mengaktifkan solenoid_lock selama INFLATING
# (state tetap seperti hasil actuator.begin(), default LOW/de-energized) -- kalau
# desain pneumatik ternyata butuh solenoid energized untuk membuka jalur inflasi,
# firmware perlu direvisi.
LOADS_INFLATING = [
    LoadSpec("ESP32-S3 aktif", 3.3, 0.080, "3.3V"),
    LoadSpec("OLED", 3.3, 0.020, "3.3V"),
    LoadSpec("ADS1115 + sensor", 5.0, 0.005, "5V"),
    LoadSpec("Pompa BLDC (peak)", 12.0, 0.400, "12V"),
    LoadSpec("Solenoid (1 aktif)", 12.0, 0.200, "12V"),
]

LOADS_HOLDING = [
    LoadSpec("MCU light sleep", 3.3, 0.005, "3.3V"),
]

BATTERY_VOLTAGE = 3.7  # LiPo nominal
BATTERY_CAPACITY_MAH = 2000

BOOST_CONVERTER_EFFICIENCY = 0.93  # MT3608, dari datasheet
LDO_EFFICIENCY_ASSUMED = 0.85       # asumsi rail 3.3V/5V lewat LDO/regulator


def battery_current_for_loads(loads: list) -> float:
    """
    Hitung total arus yang ditarik dari baterai (di rail 3.7V) untuk
    memenuhi daftar beban di berbagai rail (3.3V/5V/12V), dengan
    memperhitungkan efisiensi konversi (boost converter untuk 12V,
    LDO untuk 3.3V/5V).
    """
    total_power_from_battery_w = 0.0
    for load in loads:
        p_load = load.power_w()
        if load.rail == "12V":
            p_from_battery = p_load / BOOST_CONVERTER_EFFICIENCY
        else:
            p_from_battery = p_load / LDO_EFFICIENCY_ASSUMED
        total_power_from_battery_w += p_from_battery

    return total_power_from_battery_w / BATTERY_VOLTAGE  # I = P/V


def battery_life_hours(current_a: float, capacity_mah: float = BATTERY_CAPACITY_MAH) -> float:
    capacity_ah = capacity_mah / 1000.0
    return capacity_ah / current_a


if __name__ == "__main__":
    print("=" * 60)
    print("BAGIAN 1: PERHITUNGAN DRIVER MOSFET (IRLZ44N)")
    print("=" * 60)

    qg_total_nc = 48.0  # nC, dari datasheet IRLZ44N
    t_rise_target_us = 2.5  # target switching cepat (~5% dari periode PWM 20kHz = 50us)
    ig_ma = mosfet_gate_current(qg_total_nc, t_rise_target_us)
    print(f"Total gate charge (Qg): {qg_total_nc} nC")
    print(f"Target rise time: {t_rise_target_us} us")
    print(f"Arus gate dibutuhkan: {ig_ma:.2f} mA")
    print(f"-> Gate resistor disarankan: 100-220 ohm "
          f"(GPIO ESP32-S3 sanggup sink/source hingga 40mA/pin, aman)")
    print()

    rds_on_at_5v = 0.022  # ohm, @VGS=5V dari datasheet IRLZ44N -- REFERENSI SAJA, bukan kondisi aktual

    print(f"\nCATATAN REVISI: RDS(on)={rds_on_at_5v} ohm di atas adalah nilai @VGS=5V dari")
    print(f"datasheet IRLZ44N -- BUKAN kondisi yang dijamin pabrikan pada VGS=3.3V (GPIO ESP32-S3).")
    print(f"Sebelum memakai hasil di bawah sebagai angka final, pastikan MOSFET yang benar-benar")
    print(f"dipasang punya RDS(on) TERJAMIN datasheet pada VGS 2.5-3.3V (lihat Docs/perhitungan_driver.md")
    print(f"bagian 1), lalu ganti nilai rds_on_at_5v di script ini dengan angka RDS(on) yang sesuai.\n")

    for i_load in [0.2, 0.4]:
        p_cond = mosfet_conduction_loss(i_load, rds_on_at_5v)
        print(f"Disipasi konduksi @ I={i_load}A, Rds(on)={rds_on_at_5v} ohm (REFERENSI @VGS=5V): "
              f"{p_cond*1000:.2f} mW -> {'perlu heatsink' if p_cond > 1.0 else 'TIDAK perlu heatsink (tapi lihat catatan di atas)'}")

    print()
    print("=" * 60)
    print("BAGIAN 2: DAYA TOTAL SISTEM & UMUR BATERAI")
    print("=" * 60)

    print("\n--- Mode INFLATING ---")
    print(f"{'Komponen':<25} | {'Rail':>6} | {'Arus (mA)':>10} | {'Daya (W)':>10}")
    print("-" * 60)
    for load in LOADS_INFLATING:
        print(f"{load.name:<25} | {load.rail:>6} | {load.current_a*1000:>10.1f} | {load.power_w():>10.3f}")

    i_bat_inflating = battery_current_for_loads(LOADS_INFLATING)
    print(f"\nTotal arus baterai (3.7V) saat INFLATING: {i_bat_inflating*1000:.1f} mA")
    print("(memperhitungkan efisiensi boost converter 93% untuk rail 12V, "
          "asumsi LDO 85% untuk rail 3.3V/5V)")

    print("\n--- Mode HOLDING ---")
    i_bat_holding = battery_current_for_loads(LOADS_HOLDING)
    print(f"Total arus baterai (3.7V) saat HOLDING: {i_bat_holding*1000:.2f} mA")

    print()
    print("--- Estimasi umur baterai ---")
    t_max_continuous = battery_life_hours(i_bat_inflating)
    t_max_holding = battery_life_hours(i_bat_holding)
    print(f"Worst-case (INFLATING terus-menerus): {t_max_continuous:.2f} jam")
    print(f"  -> Target PDS spec D.1 (min. 2 jam operasi aktif): "
          f"{'TERPENUHI' if t_max_continuous >= 2.0 else 'TIDAK TERPENUHI'}")
    print(f"Best-case (HOLDING terus-menerus): {t_max_holding:.1f} jam")
    print(f"  -> Target PDS spec D.2 (min. 8 jam mode HOLDING): "
          f"{'TERPENUHI' if t_max_holding >= 8.0 else 'TIDAK TERPENUHI'}")

    print()
    print("--- Skenario realistis (siklus: 30 detik inflasi + 1 jam holding) ---")
    mah_inflating = i_bat_inflating * 1000 * (30 / 3600)
    mah_holding = i_bat_holding * 1000 * 1.0
    mah_total = mah_inflating + mah_holding
    n_cycles = BATTERY_CAPACITY_MAH / mah_total
    print(f"Kapasitas terpakai per siklus: {mah_total:.2f} mAh "
          f"(inflasi: {mah_inflating:.2f} mAh + holding 1 jam: {mah_holding:.2f} mAh)")
    print(f"Baterai {BATTERY_CAPACITY_MAH} mAh sanggup ~{n_cycles:.0f} siklus pemakaian "
          f"(skenario dominan holding)")
