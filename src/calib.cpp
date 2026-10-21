
// SPASI = simpan pose, c = kalibrasi, q = keluar

#include <opencv2/opencv.hpp>
#include <iostream>
#include <cstdlib>

int main() {
    // Pengaturan
    std::string alamatKamera = "http://127.0.0.1:4747/video";  // DroidCam 
    int kolom = 9;              
    int baris = 6;              
    float ukuranKotak = 25;      
    std::string fileKeluaran = "yaml/kalibrasi.yaml";
    size_t minimalPose = 25;
    float skalaDeteksi = 0.5;    /*deteksi di gambarnya kuperkecil jadi 0.5 soalnya kameranya delay pake droidcam, 
                                    nanti hasilnya tetep kuubah ke skala utama*/

    // intinya mengurangi delay
    setenv("OPENCV_FFMPEG_CAPTURE_OPTIONS", "fflags;nobuffer|flags;low_delay", 1);

    cv::VideoCapture cap(alamatKamera, cv::CAP_FFMPEG);
    if (!cap.isOpened()) {
        std::cerr << "Couldn't open the camera" << std::endl;
        return -1;
    }
    cap.set(cv::CAP_PROP_BUFFERSIZE, 1);

    // Titik 3D papan
    cv::Size ukuranPapan(kolom, baris);
    std::vector<cv::Point3f> titik3D;
    for (int i = 0; i < baris; i++) {
        for (int j = 0; j < kolom; j++) {
            titik3D.push_back(cv::Point3f(j * ukuranKotak, i * ukuranKotak, 0));
        }
    }

    cv::Mat frame, grayImage, tampilan;
    cv::Size ukuranGambar;
    std::vector<std::vector<cv::Point2f>> semuaTitik2D;  // sudut tiap posenya yang tersimpan

    // ngurangin delay kamera
    while (cap.isOpened()) {
        bool berhasil = true;
        while (true) {
            double awal = (double)cv::getTickCount();
            berhasil = cap.grab();
            double lamaMs = ((double)cv::getTickCount() - awal) / cv::getTickFrequency() * 1000;
            if (!berhasil || lamaMs >= 5) {
                break;
            }
        }
        if (!berhasil) {
            break;
        }
        cap.retrieve(frame);

        if (frame.empty()) {
            break;
        }

        ukuranGambar = frame.size();
        cv::cvtColor(frame, grayImage, cv::COLOR_BGR2GRAY);

        cv::Mat grayKecil;
        cv::resize(grayImage, grayKecil, cv::Size(), skalaDeteksi, skalaDeteksi);

        // FAST_CHECK: langsung menyerah kalau gaada papan
        std::vector<cv::Point2f> sudut;
        bool ketemu = cv::findChessboardCorners(
            grayKecil, ukuranPapan, sudut,
            cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE | cv::CALIB_CB_FAST_CHECK);

        tampilan = frame.clone();

        if (ketemu) {
            // mmengebalikan koordinatnya ke ukuran asli, lalu diperhalus (sub piksel)
            for (size_t i = 0; i < sudut.size(); i++) {
                sudut[i].x = sudut[i].x / skalaDeteksi;
                sudut[i].y = sudut[i].y / skalaDeteksi;
            }
            cv::cornerSubPix(grayImage, sudut, cv::Size(11, 11), cv::Size(-1, -1),
                cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.01));

            cv::drawChessboardCorners(tampilan, ukuranPapan, sudut, ketemu);
        }

        // di layarnya ijo kalau papan terdeteksi, merah kalau belum
        std::string info = "Pose: " + std::to_string(semuaTitik2D.size()) + "/" + std::to_string(minimalPose);
        cv::Scalar warnaInfo = cv::Scalar(0, 0, 255);
        if (ketemu) {
            info = info + "  [SPASI = simpan]";
            warnaInfo = cv::Scalar(0, 255, 0);
        } else {
            info = info + "  (papan belum terdeteksi)";
        }
        cv::putText(tampilan, info, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.6, warnaInfo, 2);
        cv::putText(tampilan, "c = kalibrasi   q = keluar", cv::Point(10, 60),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);

        cv::imshow("Kalibrasi", tampilan);

        int tombol = cv::waitKey(1) & 0xFF;

        if (tombol == 'q') {
            break;
        }

        if (tombol == ' ' && ketemu) {
            semuaTitik2D.push_back(sudut);
            std::cout << "Tersimpan: " << semuaTitik2D.size() << " pose" << std::endl;
        }

        if (tombol == 'c') {
            if (semuaTitik2D.size() < minimalPose) {
                std::cout << "Minimal " << minimalPose << " pose dulu" << std::endl;
            } else {
                std::vector<std::vector<cv::Point3f>> semuaTitik3D(semuaTitik2D.size(), titik3D);

                cv::Mat cameraMatrix;   // isinya fx, fy, cx, cy
                cv::Mat distCoeffs;
                std::vector<cv::Mat> rvecs, tvecs;

                // rms = error reproyeksi (piksel), makin kecil makin bagus
                double rms = cv::calibrateCamera(semuaTitik3D, semuaTitik2D, ukuranGambar,
                                                 cameraMatrix, distCoeffs, rvecs, tvecs);

                cv::FileStorage fs(fileKeluaran, cv::FileStorage::WRITE);
                if (!fs.isOpened()) {
                    std::cerr << "Gagal menyimpan ke " << fileKeluaran
                              << ". Buat dulu folder yaml nya" << std::endl;
                } else {
                    fs << "image_width" << ukuranGambar.width;
                    fs << "image_height" << ukuranGambar.height;
                    fs << "camera_matrix" << cameraMatrix;
                    fs << "dist_coeffs" << distCoeffs;
                    fs << "fx" << cameraMatrix.at<double>(0, 0);
                    fs << "fy" << cameraMatrix.at<double>(1, 1);
                    fs << "cx" << cameraMatrix.at<double>(0, 2);
                    fs << "cy" << cameraMatrix.at<double>(1, 2);
                    fs << "rms_error" << rms;
                    fs << "board_cols" << kolom;
                    fs << "board_rows" << baris;
                    fs << "square_size_mm" << ukuranKotak;
                    fs.release();

                    std::cout << "RMS error: " << rms << " px" << std::endl;
                    std::cout << "fx=" << cameraMatrix.at<double>(0, 0)
                              << " fy=" << cameraMatrix.at<double>(1, 1)
                              << " cx=" << cameraMatrix.at<double>(0, 2)
                              << " cy=" << cameraMatrix.at<double>(1, 2) << std::endl;
                    std::cout << "Disimpan ke " << fileKeluaran << std::endl;
                    break;
                }
            }
        }
    }

    cap.release();
    cv::destroyAllWindows();

    return 0;
}