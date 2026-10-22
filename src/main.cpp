/* s = simpan screenshot ke docs/ dan catat koordinat 3D ke file log
   q = keluar */
#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include "tracker.hpp"

int main() {
    std::string alamatKamera = "http://127.0.0.1:4747/video";  // DroidCam
    std::string fileYaml = "yaml/kalibrasi.yaml";

    // 1. baca hasil kalibrasi dari file YAML
    cv::FileStorage fs(fileYaml, cv::FileStorage::READ);
    if (!fs.isOpened()) {
        std::cerr << "file kalibrasinya gabisa dibuka" << std::endl;
        return -1;
    }

    // camera_matrix berukuran 3x3:
    //   fx  0  cx
    //    0 fy  cy
    //    0  0   1
    cv::Mat cameraMatrix;
    fs["camera_matrix"] >> cameraMatrix;
    fs.release();

    if (cameraMatrix.empty()) {
        std::cerr << "camera_matrix tidak ditemukan di file yaml" << std::endl;
        return -1;
    }

    double fx = cameraMatrix.at<double>(0, 0);
    double fy = cameraMatrix.at<double>(1, 1);
    double cx = cameraMatrix.at<double>(0, 2);
    double cy = cameraMatrix.at<double>(1, 2);
    std::cout << "fx=" << fx << " fy=" << fy << " cx=" << cx << " cy=" << cy << std::endl;

    // 2. buka kamera
    cv::VideoCapture cap(alamatKamera, cv::CAP_FFMPEG);
    if (!cap.isOpened()) {
        std::cerr << "kameranya gabisa dibuka woi" << std::endl;
        return -1;
    }
    cap.set(cv::CAP_PROP_BUFFERSIZE, 1);  // biar gak delay

    cv::Mat frame;

    // 3. nyiapin 3 objeknya
    const int jumlahObjek = 3;
    Objek objek[jumlahObjek] = {
        {"kuning kuning", 6.0,  cv::Scalar(0, 255, 255), 18, 35, 70, 200, 120, 255},
        {"toples ungu",   11.3, cv::Scalar(255, 80, 200), 100, 135, 40, 200, 60, 255},
        {"bolalala oren",         22.5, cv::Scalar(0, 140, 255), 0, 14, 40, 170, 100, 255}
    };

    // 4. buat jendela dan trackbar
    int batasDekat = 30;   // kalau Z lebih kecil dari ini (cm) -> peringatan
    int luasMin = 1500;    // kontur lebih kecil dari ini dianggap noise

    cv::namedWindow("Kontrol");
    cv::createTrackbar("Dekat < (cm)", "Kontrol", &batasDekat, 300);
    cv::createTrackbar("Luas min", "Kontrol", &luasMin, 20000);

    // buat jendela slider HLS untuk setiap objek
    for (int i = 0; i < jumlahObjek; i++) {
        std::string namaJendela = "HLS - " + objek[i].nama;
        cv::namedWindow(namaJendela);
        cv::createTrackbar("H min", namaJendela, &objek[i].hMin, 180);
        cv::createTrackbar("H max", namaJendela, &objek[i].hMax, 180);
        cv::createTrackbar("L min", namaJendela, &objek[i].lMin, 255);
        cv::createTrackbar("L max", namaJendela, &objek[i].lMax, 255);
        cv::createTrackbar("S min", namaJendela, &objek[i].sMin, 255);
        cv::createTrackbar("S max", namaJendela, &objek[i].sMax, 255);
    }

    cv::Mat hlsImage;
    cv::Mat result;
    cv::Mat gabunganMask;
    double fps = 0;
    double waktuAwal = (double)cv::getTickCount();

    // 5. loop utama
    while (cap.isOpened()) {
        cap >> frame;

        if (frame.empty()) {
            break;
        }

        // convert color: BGR -> HLS
        cv::cvtColor(frame, hlsImage, cv::COLOR_BGR2HLS);

        // deteksi tiap objek (threshold, morfologi, kontur, bounding box, XYZ)
        for (int i = 0; i < jumlahObjek; i++) {
            deteksiObjek(hlsImage, objek[i], fx, fy, cx, cy, luasMin);
        }

        // gambar hasilnya di copy an frame
        result = frame.clone();
        for (int i = 0; i < jumlahObjek; i++) {
            if (!objek[i].ketemu) {
                continue;
            }

            // kalau terlalu dekat, kotaknya jadi merah
            cv::Scalar warna = objek[i].warna;
            bool terlaluDekat = objek[i].Z < batasDekat;
            if (terlaluDekat) {
                warna = cv::Scalar(0, 0, 255);
            }

            cv::rectangle(result, objek[i].kotak, warna, 2);
            cv::circle(result, cv::Point(objek[i].xPixel, objek[i].yPixel), 5, warna, cv::FILLED);

            // Teks di atas kotak 
            std::string teks1 = objek[i].nama + "  Z = " + cv::format("%.1f", objek[i].Z) + " cm";
            std::string teks2 = "(X,Y,Z) = (" + cv::format("%.1f", objek[i].X) + ", "
                              + cv::format("%.1f", objek[i].Y) + ", "
                              + cv::format("%.1f", objek[i].Z) + ") cm";

            // Kalo kotak terlalu dekat sama tepi atas layar, teksny ditaruh di bawah kotak
            int yTeks = objek[i].kotak.y - 28;
            if (yTeks < 20) {
                yTeks = objek[i].kotak.y + objek[i].kotak.height + 20;
            }
            cv::putText(result, teks1, cv::Point(objek[i].kotak.x, yTeks),
                        cv::FONT_HERSHEY_SIMPLEX, 0.55, warna, 2);
            cv::putText(result, teks2, cv::Point(objek[i].kotak.x, yTeks + 20),
                        cv::FONT_HERSHEY_SIMPLEX, 0.55, warna, 2);

            if (terlaluDekat) {
                cv::putText(result, "DEKAT!",
                            cv::Point(objek[i].kotak.x, objek[i].kotak.y + objek[i].kotak.height + 40),
                            cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
            }
        }

        // hitung fps
        double waktuSekarang = (double)cv::getTickCount();
        double selisih = (waktuSekarang - waktuAwal) / cv::getTickFrequency();  // dalam detik
        waktuAwal = waktuSekarang;
        if (selisih > 0) {
            fps = 1.0 / selisih;
        }
        cv::putText(result, "FPS: " + cv::format("%.0f", fps), cv::Point(10, 25),
                    cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
        cv::putText(result, "s = simpan   q = keluar", cv::Point(10, 55),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);

        // gabungin mask 3 objek jadi satu 
        cv::bitwise_or(objek[0].mask, objek[1].mask, gabunganMask);
        cv::bitwise_or(gabunganMask, objek[2].mask, gabunganMask);

        cv::imshow("Real Image", frame);
        cv::imshow("Mask", gabunganMask);
        cv::imshow("Bounding Box Result", result);

        // "& 0xFF" biar hasilnya benar meskpun numlocknya nyala
        int tombol = cv::waitKey(25) & 0xFF;

        if (tombol == 'q') {
            break;
        }

        if (tombol == 's') {
            // save screenshot di docs
            bool tersimpan = cv::imwrite("docs/objek_detect.png", result);
            if (!tersimpan) {
                std::cerr << "Gagal menyimpan gambar. Buat dulu folder docs nya" << std::endl;
            } else {
                // save log koordinat 3D ke file txt
                std::ofstream logFile("docs/log_koordinat.txt", std::ios::app);
                for (int i = 0; i < jumlahObjek; i++) {
                    if (objek[i].ketemu) {
                        logFile << objek[i].nama
                                << " : X = " << cv::format("%.1f", objek[i].X)
                                << " cm, Y = " << cv::format("%.1f", objek[i].Y)
                                << " cm, Z = " << cv::format("%.1f", objek[i].Z) << " cm" << std::endl;
                    }
                }
                logFile << "-----" << std::endl;
                std::cout << "Tersimpan di folder docs" << std::endl;
            }
        }
    }

    cap.release();
    cv::destroyAllWindows();

    std::cout << "Batas HLS terakhir:" << std::endl;
    for (int i = 0; i < jumlahObjek; i++) {
        std::cout << objek[i].nama << ": H " << objek[i].hMin << "-" << objek[i].hMax
                  << ", L " << objek[i].lMin << "-" << objek[i].lMax
                  << ", S " << objek[i].sMin << "-" << objek[i].sMax << std::endl;
    }

    return 0;
}