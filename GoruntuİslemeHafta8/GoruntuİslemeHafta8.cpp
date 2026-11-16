#include <opencv2/opencv.hpp>
#include <iostream>
#include <cmath>
#include <algorithm>

using namespace cv;
using namespace std;

Mat hsiDonustur(const Mat& resim)
{
    Mat sonuc(resim.size(), CV_8UC3);

    for (int y = 0; y < resim.rows; y++)
    {
        for (int x = 0; x < resim.cols; x++)
        {
            Vec3b p = resim.at<Vec3b>(y, x);

            double B = p[0] / 255.0;
            double G = p[1] / 255.0;
            double R = p[2] / 255.0;

            double toplam = R + G + B;
            double I = toplam / 3.0;
            double S = 0.0;
            double H = 0.0;

            if (toplam > 0.0)
                S = 1.0 - 3.0 * min(R, min(G, B)) / toplam;

            double pay = 0.5 * ((R - G) + (R - B));

            double paydaKare =
                (R - G) * (R - G) +
                (R - B) * (G - B);

            if (paydaKare > 1e-12)
            {
                double oran = pay / sqrt(paydaKare);
                oran = max(-1.0, min(1.0, oran));

                H = acos(oran);

                if (B > G)
                    H = 2.0 * CV_PI - H;
            }

            // H, S ve I degerlerini 8 bite kodla.
            sonuc.at<Vec3b>(y, x) = Vec3b(
                saturate_cast<uchar>(H * 255.0 / (2.0 * CV_PI)),
                saturate_cast<uchar>(S * 255.0),
                saturate_cast<uchar>(I * 255.0)
            );
        }
    }

    return sonuc;
}

int main()
{
    try
    {
        Mat kaynak = imread("insanlar.jpeg", IMREAD_COLOR);

        if (kaynak.empty())
        {
            cerr << "insanlar.jpeg bulunamadi!" << endl;
            return -1;
        }

        if (!imwrite("gorev4_orijinal.bmp", kaynak))
            return -1;

        Mat resim = imread("gorev4_orijinal.bmp", IMREAD_COLOR);

        if (resim.empty())
            return -1;

        Mat hsv, hsi, yuv, lab;

        // OpenCV renkli resmi BGR sirasiyla okur.
        cvtColor(resim, hsv, COLOR_BGR2HSV);
        hsi = hsiDonustur(resim);
        cvtColor(resim, yuv, COLOR_BGR2YUV);
        cvtColor(resim, lab, COLOR_BGR2Lab);

        bool kaydedildi = true;
        kaydedildi &= imwrite("gorev4_hsv.bmp", hsv);
        kaydedildi &= imwrite("gorev4_hsi.bmp", hsi);
        kaydedildi &= imwrite("gorev4_yuv.bmp", yuv);
        kaydedildi &= imwrite("gorev4_lab.bmp", lab);

        if (!kaydedildi)
        {
            cerr << "Sonuclar kaydedilemedi!" << endl;
            return -1;
        }

        cout << "Boyut: " << resim.cols
            << " x " << resim.rows << endl;
        cout << "HSV, HSI, YUV ve Lab dosyalari kaydedildi."
            << endl;

        namedWindow("Orijinal", WINDOW_NORMAL);
        namedWindow("HSV - Kodlanmis Kanallar", WINDOW_NORMAL);
        namedWindow("HSI - Kodlanmis Kanallar", WINDOW_NORMAL);
        namedWindow("YUV - Kodlanmis Kanallar", WINDOW_NORMAL);
        namedWindow("Lab - Kodlanmis Kanallar", WINDOW_NORMAL);

        imshow("Orijinal", resim);
        imshow("HSV - Kodlanmis Kanallar", hsv);
        imshow("HSI - Kodlanmis Kanallar", hsi);
        imshow("YUV - Kodlanmis Kanallar", yuv);
        imshow("Lab - Kodlanmis Kanallar", lab);

        waitKey(0);
    }
    catch (const exception& hata)
    {
        cerr << hata.what() << endl;
        return -1;
    }

    return 0;
}