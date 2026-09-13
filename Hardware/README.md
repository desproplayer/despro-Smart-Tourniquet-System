# 🔌 Hardware

- `Schematics/` — file KiCad (.kicad_pro, .kicad_sch, .kicad_pcb) + ekspor Gerber untuk fabrikasi PCB custom 2-layer (100x70mm, JLCPCB)
- `BOM/` — Bill of Materials (lihat juga RAB di Docs/Laporan_Kemajuan)

Rekomendasi workflow (lihat diskusi simulasi):
1. Validasi logic dulu di **Wokwi** (model virtual MPX5050DP via Custom Chips API karena bukan komponen drag-and-drop bawaan)
2. Skematik final + PCB di **KiCad**
3. (Opsional) validasi analog/power lebih detail di **Proteus**
