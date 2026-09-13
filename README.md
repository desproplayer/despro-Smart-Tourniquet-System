# 🩸 Smart Tourniquet System for Rapid Hemorrhage Control in Disaster Victims

> Proyek Desain Proyek Teknik Elektro, Komputer, Biomedik 2 — Universitas Indonesia (2026/2027)
> Sistem tourniquet cerdas hibrida (pneumatik otomatis + cadangan mekanik) dengan **Focal Pressure Pad**
> dan algoritma **Limb Occlusion Pressure (LOP)** closed-loop, untuk penghentian perdarahan masif
> di lapangan bencana. Klien: Basarnas / BNPB.

## 📖 Table of Contents
- [About the Project](#about-the-project)
- [Key Features](#key-features)
- [Team Members](#team-members)
- [System Architecture](#system-architecture)
- [Technology Stack](#technology-stack)
- [Repository Structure](#repository-structure)
- [Getting Started](#getting-started)
- [License](#license)

## 🔬 About the Project

Tourniquet konvensional (mis. CAT windlass) menekan seluruh keliling ekstremitas, ikut mematikan
suplai darah ke saraf dan jaringan sehat, dan berisiko overtightening oleh operator non-medis.
Proyek ini mengembangkan **Focal Pressure Pad (FPP)** — kantung tekanan oval yang hanya menekan
titik arteri utama (brachial/femoral) — dikombinasikan dengan algoritma **LOP oscillometric
closed-loop** berbasis ESP32-S3, sehingga tekanan minimal efektif tercapai otomatis tanpa merusak
jaringan di sekitarnya.

Sistem melakukan inflasi bertahap, mendeteksi hilangnya osilasi arteri (pipeline filter Butterworth
+ peak detection), mengunci tekanan via solenoid saat LOP tercapai, memonitor kebocoran/pelonggaran
setiap 15 detik dengan reinflasi otomatis, dan menyediakan mode T-Conversion (slow-release
terkontrol) untuk penanganan di IGD.

## ✨ Key Features
- **Focal Pressure Pad** — kompresi lokal pada jalur arteri, bukan sirkumferensial penuh
- **Algoritma LOP Closed-Loop** — deteksi oklusi otomatis via oscillometric (Butterworth orde 5, 1–5 Hz + peak detection)
- **Monitoring Aktif** — reinflasi otomatis bila tekanan drop >15 mmHg, alarm kebocoran bila drop >30 mmHg/5 detik
- **Fail-Safe Mekanik** — tuas manual untuk pelepasan darurat <3 detik tanpa listrik
- **T-Conversion** — mode slow-release terkontrol (20 mmHg/menit) untuk tenaga medis di IGD
- **UI Non-Medis** — OLED, buzzer, LED tiga warna, dua tombol (START/RELEASE), waktu belajar <30 detik

## 👥 Team Members — Kelompok 19

| Nama | Program Studi | Peran |
|---|---|---|
| Dhafin Hamizan Setiawan | Teknik Komputer | Ketua Kelompok — Firmware & Logic |
| Grace Kezia Siregar | Teknik Biomedik | Biomekanika FPP, Parameter LOP, Validasi Sensor & Phantom, FMEA |
| Michelle Winona Joyceline Simamora | Teknik Biomedik | UI/UX, Interface Klinis |
| Daffa Bagus Dhiananto | Teknik Komputer | Firmware, DSP, State Machine |
| Drina Shahada Wibowo | Teknik Elektro | Hardware, PCB, Manajemen Daya |

**Dosen Pembimbing:** Siti Fauziyah Rahman, S.T., M.Eng., Ph.D

## 📐 System Architecture

```
MPX5050DP (0-375 mmHg) ── AFE (RC filter) ── ADS1115 ── I2C ── ESP32-S3
                                                                    │
                                                     ┌──────────────┼──────────────┐
                                                     │              │              │
                                              Butterworth      PID Control    State Machine
                                              BPF 1-5 Hz       (pump PWM)     (UI/OLED/BLE)
                                                     │              │
                                              Peak Detection   MOSFET Driver
                                                     │              │
                                              LOP Decision  ──> Pump / Solenoid Valve (x2)
```

## 🛠 Technology Stack
- **Mikrokontroler:** ESP32-S3 (240 MHz, FPU, BLE 5.0)
- **Firmware:** C/C++ — ESP-IDF (FreeRTOS tasks) atau Arduino framework via PlatformIO
- **Sensor:** MPX5050DP (analog differential, 0–375 mmHg) + ADS1115 (ADC eksternal 16-bit)
- **Simulasi/Validasi Logic:** Wokwi (model virtual sensor via Custom Chips API)
- **Skematik & PCB:** KiCad (custom PCB 2-layer, 100×70mm)
- **Simulasi Analog (opsional):** Proteus
- **Desain Mekanik:** Autodesk Fusion 360 (casing IP54, Focal Pressure Pad)
- **FEM Validasi Tekanan:** SolidWorks Simulation / Abaqus

## 📂 Repository Structure

```
📦 smart-tourniquet-system
 ┣ 📂 Algorithms       # Python/MATLAB: desain filter Butterworth, simulasi LOP, model sensor
 ┣ 📂 Docs             # Datasheet, proposal, laporan kemajuan pekanan, referensi jurnal
 ┣ 📂 Hardware
 ┃ ┣ 📂 Schematics     # File KiCad (.kicad_sch, .kicad_pcb) dan ekspor Gerber
 ┃ ┗ 📂 BOM            # Bill of Materials
 ┣ 📂 Mechanical       # File CAD Fusion 360 — casing Smart Module & Focal Pressure Pad
 ┣ 📂 firmware
 ┃ ┗ 📂 esp32-arduino-pio   # Firmware ESP32-S3 (PlatformIO, Arduino framework)
 ┣ 📜 .gitignore
 ┣ 📜 LICENSE
 ┗ 📜 README.md
```

## 🚀 Getting Started

### Prerequisites
- [VS Code](https://code.visualstudio.com/) + ekstensi **PlatformIO IDE**
- **Git**
- (Opsional) akun [Wokwi](https://wokwi.com/) untuk simulasi sebelum flash ke hardware asli

### Installation & Build
1. **Clone repository:**
   ```bash
   git clone https://github.com/<username-kamu>/smart-tourniquet-system.git
   cd smart-tourniquet-system/firmware/esp32-arduino-pio
   ```
2. **Buka folder di VS Code** dengan ekstensi PlatformIO aktif — PlatformIO otomatis mengunduh
   toolchain ESP32-S3 dan library (`Adafruit_ADS1X15`, `Adafruit_SSD1306`) sesuai `platformio.ini`.
3. **Build:** klik ikon centang (✓) PlatformIO, atau `pio run` dari terminal.
4. **Upload ke ESP32-S3:** sambungkan board via USB-C, klik ikon panah (→) PlatformIO, atau `pio run -t upload`.
5. **Monitor serial:** `pio device monitor` (baudrate 115200).

## ⚖️ License
Proyek akademik — Desain Proyek Teknik Biomedik/Elektro/Komputer, Fakultas Teknik Universitas Indonesia.
Lisensi MIT untuk kode firmware (lihat `LICENSE`), kecuali dinyatakan lain.
