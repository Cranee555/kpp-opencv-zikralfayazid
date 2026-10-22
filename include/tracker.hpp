/*
 * isi hppnya adalah struct Objek dan fungsi deteksiObjek()
 * Convert Color -> Thresholding -> Morphology -> Contour -> Bounding Box
 * dan juga menghitung posisi 3D (X, Y, Z).
 */

#pragma once

#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>

struct Objek {
    std::string nama;       // contoh: "toples ungu"
    double lebarAsli;       // lebar asli benda (cm)
    cv::Scalar warna;       // warna kotak di layar, urutannya B, G, R

    // Batas threshold HLS (bisa diubah lewat trackbar)
    int hMin, hMax;
    int lMin, lMax;
    int sMin, sMax;


    bool ketemu = false;
    cv::Rect kotak;         // bounding box: x, y, width, height
    int xPixel = 0;         // titik tengah sumbu x dalam piksel
    int yPixel = 0;         // titik tengah sumbu y dalam piksel
    double X = 0;
    double Y = 0;
    double Z = 0;
    cv::Mat mask;
};

//   luasMin  : kontur yang lebih kecil dari ini dianggap noise
void deteksiObjek(const cv::Mat& hlsImage, Objek& obj,
                  double fx, double fy, double cx, double cy, int luasMin) {

    obj.ketemu = false;

    // 1. thresholding
    cv::inRange(hlsImage,
                cv::Scalar(obj.hMin, obj.lMin, obj.sMin),
                cv::Scalar(obj.hMax, obj.lMax, obj.sMax),
                obj.mask);

    // 2. morphological erode dan dilate
    // erode terakhir 6 (bukan 5) biar totalnya pas (2 - 8 + 6 = 0),
    // jadi bounding box gak membesar dan jarak Z lebih akurat
    cv::erode(obj.mask, obj.mask, cv::Mat(), cv::Point(-1, -1), 2);
    cv::dilate(obj.mask, obj.mask, cv::Mat(), cv::Point(-1, -1), 8);
    cv::erode(obj.mask, obj.mask, cv::Mat(), cv::Point(-1, -1), 6);

    // 3. contour detection
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(obj.mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // 4. pilih kontur yang terbesar (yang luasnya lebih dari luasMin)
    int indeksTerbesar = -1;
    double luasTerbesar = 0;
    for (size_t i = 0; i < contours.size(); i++) {
        double luas = cv::contourArea(contours[i]);
        if (luas > luasMin && luas > luasTerbesar) {
            luasTerbesar = luas;
            indeksTerbesar = (int)i;
        }
    }

    // Kalau gaada kontur yang cukup besar, objeknya dianggap tidak ada
    if (indeksTerbesar < 0) {
        return;
    }

    // 5. bounding box dan titik tengah
    obj.kotak = cv::boundingRect(contours[indeksTerbesar]);
    if (obj.kotak.width <= 0) {
        return;
    }
    obj.xPixel = obj.kotak.x + obj.kotak.width / 2;
    obj.yPixel = obj.kotak.y + obj.kotak.height / 2;

    // 6. hitung posisi 3D
    obj.Z = fx * obj.lebarAsli / obj.kotak.width;
    obj.X = (obj.xPixel - cx) * obj.Z / fx;
    obj.Y = (obj.yPixel - cy) * obj.Z / fy;
    // semua satuannya cm karena lebarAsli juga dalam cm

    obj.ketemu = true;
}