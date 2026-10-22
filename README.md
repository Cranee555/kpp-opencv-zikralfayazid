# kpp-opencv-zikralfayazid

Nama : Zikral Fayazid
NRP : 5054261007

Analisis : Kamera yang sudah dikalibrasi akan menghasilkan fx, fy, cx, dan cy yang nantinya dijadikan acuan pada kode main.cpp. Setelah itu setiap frame dari kamera diubah ke format HLS, lalu di threshold untuk misahin warna tiap objeknya, dan noise dibersihkan dengan erode dan dilate. Dari mask yang bersih dicari kontur terbesarnya, lalu dibuat bounding box beserta titik tengahnya (x_pixel, y_pixel). Lebar bounding box (w_pixel) dan lebar asli objek (W_real) dipakai untuk menghitung jarak Z = (fx * W_real) / w_pixel, kemudian posisi X dan Y dihitung dari selisih titik tengah objek terhadap pusat optik kamera (cx, cy). Hasilnya ditampilkan di layar berupa bounding box, label nama, jarak Z, koordinat (X, Y, Z), dan FPS, serta ditandai merah jika objek terlalu dekat.

Kendala : Saat melakukan kalibrasi dan membuat kode calib.cpp, kamera nya delay saat melakukan kalibrasi sehingga saya membuat skala kamera untuk pendeteksi chessboardnya menjadi 0.5 agar tidak terjadi delay, setelah itu nanti koordinat hasil kalibrasinya diubah lagi menjadi skala awal agar tetap akurat

Cara Penggunaan : 
1. connect in kamera hp ke ubuntu dengan menggunakan droidcam, usahakan device terdeteksi dengan command terminal:
adb devices
kalo terdeteksi device hp yang akan di konek in, lanjut ke command berikutnya:
adb forward tcp:4747 tcp:4747
maka kamera siap untuk digunakan
2. Melakukan kalibrasi kamera terlebih dahulu (calib.cpp) dengan command terminal :
g++ -std=c++17 src/calib.cpp -o calib $(pkg-config --cflags --libs opencv4)
./calib
usahakan sudah membuat folder yaml nya terlebih dahulu, karena untuk menyimpan file yaml hasil kalibrasi harus membuat folder yaml terlebih dahulu (kalo belum membuatnya nanti di kodenya ada pengecekan untuk itu yang memberi tahu kalo semisal folder yaml belum dibuat)
3. Jalankan file main.cpp di terminal dengan command:
g++ -std=c++17 src/main.cpp -Iinclude -o main $(pkg-config --cflags --libs opencv4)
./main
setelah main.cpp berhasil di run, maka akan muncul layar hp kita di ubuntu, dan coba arahkan ke 3 objek yang udah kita tentuin HLS nya, jika kita ingin save foto dan juga koordinat dari objek tersebut, pencet "s" dan jika ingin keluar penect "q" atau "ctrl + C" di terminal. Usahakan sudah membuat file docs/ sebelumnya karena jika tidak maka foto dan koordinat gabakal kesimpan dan muncul peringatan untuk membuat file docs terlebih dahulu. 

NOTE: JIKA SUDAH MENGCAPTURE FOTO DARI SUATU OBJEK DENGAN "s" DI main.cpp, UBAHLAH NAMA FILE .png AGAR TIDAK KE OVERWRITE DAN HILANG SAAT PENGAMBILAN FOTO OBJEK SETELAHNYA


