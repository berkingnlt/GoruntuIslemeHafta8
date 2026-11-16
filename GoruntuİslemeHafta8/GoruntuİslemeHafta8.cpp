#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <stdexcept>

using namespace cv;
using namespace std;

bool rgbTenMi(const Vec3b& p)
{
    int B = p[0];
    int G = p[1];
    int R = p[2];

    int enBuyuk = max(R, max(G, B));
    int enKucuk = min(R, min(G, B));

    return R > 50 && G > 20 && B > 10 &&
        enBuyuk - enKucuk > 15 &&
        abs(R - G) > 10 &&
        R > G && R > B;
}

bool ycrcbTenMi(const Vec3b& p)
{
    // OpenCV kanal sirasi: Y, Cr, Cb.
    int Cr = p[1];
    int Cb = p[2];

    return Cr >= 133 && Cr <= 173 &&
        Cb >= 77 && Cb <= 127;
}

bool hsvTenMi(const Vec3b& p)
{
    int H = p[0];
    int S = p[1];
    int V = p[2];

    return H <= 25 &&
        S >= 40 && S <= 230 &&
        V >= 35;
}

void sonucuKaydet(
    const Mat& orijinal,
    const Mat& maske,
    const string& isim,
    ofstream& rapor)
{
    Mat sonuc = orijinal.clone();

    for (int y = 0; y < sonuc.rows; y++)
    {
        for (int x = 0; x < sonuc.cols; x++)
        {
            if (maske.at<uchar>(y, x) == 0)
                sonuc.at<Vec3b>(y, x) = Vec3b(0, 0, 255);
        }
    }

    if (!imwrite("gorev5_" + isim + "_sonuc.bmp", sonuc) ||
        !imwrite("gorev5_" + isim + "_maske.bmp", maske))
    {
        throw runtime_error("Sonuclar kaydedilemedi.");
    }

    int tenPikseli = countNonZero(maske);
    double oran = 100.0 * tenPikseli / maske.total();

    cout << isim << " ten adayi: "
        << tenPikseli << " piksel, %" << oran << endl;

    rapor << isim << " ten adayi: "
        << tenPikseli << " piksel, %" << oran << "\n";

    namedWindow(isim + " Sonuc", WINDOW_NORMAL);
    namedWindow(isim + " Maske", WINDOW_NORMAL);

    imshow(isim + " Sonuc", sonuc);
    imshow(isim + " Maske", maske);
}

int main()
{
    try
    {
        Mat resim = imread("insanlar.jpeg", IMREAD_COLOR);

        if (resim.empty())
            throw runtime_error("insanlar.jpeg bulunamadi!");

        Mat ycrcb, hsv;

        cvtColor(resim, ycrcb, COLOR_BGR2YCrCb);
        cvtColor(resim, hsv, COLOR_BGR2HSV);

        Mat rgbMaske = Mat::zeros(resim.size(), CV_8UC1);
        Mat ycrcbMaske = Mat::zeros(resim.size(), CV_8UC1);
        Mat hsvMaske = Mat::zeros(resim.size(), CV_8UC1);

        for (int y = 0; y < resim.rows; y++)
        {
            for (int x = 0; x < resim.cols; x++)
            {
                rgbMaske.at<uchar>(y, x) =
                    rgbTenMi(resim.at<Vec3b>(y, x)) ? 255 : 0;

                ycrcbMaske.at<uchar>(y, x) =
                    ycrcbTenMi(ycrcb.at<Vec3b>(y, x)) ? 255 : 0;

                hsvMaske.at<uchar>(y, x) =
                    hsvTenMi(hsv.at<Vec3b>(y, x)) ? 255 : 0;
            }
        }

        ofstream rapor("gorev5_karsilastirma.txt");

        if (!rapor)
            throw runtime_error("Rapor olusturulamadi.");

        rapor << "TEN TESPITI KARSILASTIRMASI\n\n";

        rapor << "RGB: R>50, G>20, B>10, max-min>15, "
            << "|R-G|>10, R>G, R>B.\n";

        rapor << "YCbCr: 133<=Cr<=173, 77<=Cb<=127.\n";

        rapor << "HSV: 0<=H<=25, 40<=S<=230, V>=35.\n\n";

        if (!imwrite("gorev5_orijinal.bmp", resim))
            throw runtime_error("Orijinal kaydedilemedi.");

        sonucuKaydet(resim, rgbMaske, "RGB", rapor);
        sonucuKaydet(resim, ycrcbMaske, "YCbCr", rapor);
        sonucuKaydet(resim, hsvMaske, "HSV", rapor);

        rapor
            << "\nYORUM:\n"
            << "Ten adayi pikseller orijinal renkleriyle "
            << "korunmus, diger pikseller tam kirmizi yapilmistir.\n"
            << "Maskelerde beyaz ten adayi, siyah ten disidir.\n\n"
            << "RGB'de parlaklik ve renk bilgisi birlikte "
            << "degistigi icin sabit esikler isiktan etkilenir.\n"
            << "YCbCr'de renk farki kanallari, HSV'de ise "
            << "ton ve doygunluk bilgisi kullanilabilir.\n"
            << "Bu ayrimlar ten tespitini kolaylastirabilir; "
            << "ancak aydinlatmadan tam bagimsizlik saglamaz.\n\n"
            << "Kullanilan esikler baslangic degerleridir. "
            << "Farkli ten tonlari ve golgeler kacirilabilir; "
            << "ten rengine benzeyen kiyafet veya arka planlar "
            << "yanlis secilebilir.\n"
            << "Secilen piksel yuzdesi basari orani degildir. "
            << "Dogruluk icin elle etiketlenmis referans maske gerekir.\n";

        rapor.close();

        namedWindow("Orijinal", WINDOW_NORMAL);
        imshow("Orijinal", resim);

        waitKey(0);
    }
    catch (const exception& hata)
    {
        cerr << "Hata: " << hata.what() << endl;
        return -1;
    }

    return 0;
}