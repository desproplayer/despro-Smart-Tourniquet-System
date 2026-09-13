# 📚 Docs

Folder ini menyimpan dokumen non-kode proyek:

- `Laporan_Desain_Proyek.pdf` — proposal desain utama (Bab I-VII + PDS)
- `Laporan_Kemajuan_Week02.pdf`, `Laporan_Kemajuan_Week03.pdf`, dst. — laporan kemajuan pekanan
- `MPX5050DP_datasheet.pdf` — datasheet sensor tekanan (Motorola/NXP)
- `perhitungan_sensor.md` — turunan rumus transfer function MPX5050DP (Vout <-> mmHg), catatan
  saturasi sensor di 375 mmHg vs target hard limit 400 mmHg di PDS, dan perhitungan error akurasi
  vs target ±2 mmHg
- `referensi/` — kumpulan sitasi jurnal (Vega et al. 2022, Snider et al. 2022, McEwen et al. 2015,
  Masri et al. 2020, dll.) dan pembanding tesis/produk sejenis (mis. tourniquet digital berbasis
  MPX5050GP oleh Fahru, 2022)

> Catatan: dokumen PDF asli sebaiknya di-upload manual lewat GitHub web UI atau `git lfs` jika
> ukurannya besar, supaya riwayat commit tidak bengkak.
