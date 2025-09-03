# ESP8266-Deauther-Evil-Twin

/*
=====================================================================
Program Deauther & Evil Twin ESP8266
=====================================================================


Program ini digunakan untuk membuat alat jaringan WiFi berbasis ESP8266 
dengan dua fungsi utama: 
1. **Deauther** → Mengirim paket deauthentication untuk memutus koneksi 
   perangkat klien dari AP target.  
2. **Evil Twin (Rogue AP)** → Membuat Access Point palsu dengan SSID 
   yang dipilih untuk mengetahui password wifi.

Konfigurasi Variabel:
- `String AP`       : SSID (nama WiFi) untuk AP web control alat → **WAJIB diubah**    
- `String AP_Pass`  : Password AP web control → **WAJIB diubah**   
- `indihome.png`    : File gambar yang digunakan pada halaman web phising, 
  disimpan dan diakses melalui **LittleFS**.  

Fitur Utama:
1. **Web Control Panel**
   - Halaman ini digunakan untuk mengatur serangan deauth atau mengaktifkan 
     AP phising.  
   - Akses halaman memerlukan password (`AP_Pass`).  

2. **Evil Twin Attack**
   - Membuat AP palsu yang meniru SSID target (misalnya jaringan publik).  
   - Halaman login palsu menampilkan file `indihome.png` sebagai bagian dari 
     web phising (sudah diupload ke LittleFS).  

3. **Deauthentication Attack**
   - Mengirimkan paket deauth untuk memutuskan perangkat klien dari AP asli.  
   - Digunakan sebagai simulasi/testing keamanan jaringan WiFi.  

Tujuan:
- Memberikan contoh implementasi serangan **WiFi Deauther & Evil Twin** 
  untuk pembelajaran keamanan jaringan (network security learning).  
- Menggunakan ESP8266 sebagai alat uji penetrasi sederhana.  

Catatan Penting:
⚠️ Program ini hanya boleh digunakan untuk tujuan edukasi dan riset 
keamanan jaringan pribadi. Penyalahgunaan di jaringan orang lain 
melanggar hukum.  
=====================================================================
*/
