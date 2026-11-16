#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

using namespace cv;
using namespace std;

void dosyalariBul(vector<String>& dosyalar)
{
    vector<String> bulunanlar;

    const string uzantilar[] = {
        "*.jpg", "*.jpeg", "*.png", "*.bmp", "*.ppm"
    };

    for (const string& uzanti : uzantilar)
    {
        bulunanlar.clear();
        glob("trafik/" + uzanti, bulunanlar, true);

        dosyalar.insert(
            dosyalar.end(),
            bulunanlar.begin(),
            bulunanlar.end()
        );
    }

    sort(dosyalar.begin(), dosyalar.end());

    dosyalar.erase(
        unique(dosyalar.begin(), dosyalar.end()),
        dosyalar.end()
    );
}

void kirmiziMaskeleri(
    const Mat& resim,
    Mat& rgbMaske,
    Mat& hsvMaske,
    Mat& labMaske,
    Mat& ortakMaske)
{
    Mat hsv, lab;
    cvtColor(resim, hsv, COLOR_BGR2HSV);
    cvtColor(resim, lab, COLOR_BGR2Lab);

    rgbMaske = Mat::zeros(resim.size(), CV_8UC1);
    hsvMaske = Mat::zeros(resim.size(), CV_8UC1);
    labMaske = Mat::zeros(resim.size(), CV_8UC1);
    ortakMaske = Mat::zeros(resim.size(), CV_8UC1);

    for (int y = 0; y < resim.rows; y++)
    {
        for (int x = 0; x < resim.cols; x++)
        {
            Vec3b p = resim.at<Vec3b>(y, x);
            Vec3b h = hsv.at<Vec3b>(y, x);
            Vec3b l = lab.at<Vec3b>(y, x);

            int B = p[0], G = p[1], R = p[2];

            bool rgbKirmizi =
                R > 70 &&
                R > 1.3 * G &&
                R > 1.3 * B;

            bool hsvKirmizi =
                (h[0] <= 10 || h[0] >= 170) &&
                h[1] >= 80 &&
                h[2] >= 50;

            bool labKirmizi =
                l[1] >= 150 &&
                l[2] >= 120;

            rgbMaske.at<uchar>(y, x) = rgbKirmizi ? 255 : 0;
            hsvMaske.at<uchar>(y, x) = hsvKirmizi ? 255 : 0;
            labMaske.at<uchar>(y, x) = labKirmizi ? 255 : 0;

            int oy =
                static_cast<int>(rgbKirmizi) +
                static_cast<int>(hsvKirmizi) +
                static_cast<int>(labKirmizi);

            ortakMaske.at<uchar>(y, x) = oy >= 2 ? 255 : 0;
        }
    }

    Mat cekirdek = getStructuringElement(
        MORPH_ELLIPSE, Size(3, 3)
    );

    morphologyEx(
        ortakMaske, ortakMaske,
        MORPH_CLOSE, cekirdek
    );
}

double cevreKirmiziOrani(const Mat& maske, const Vec3f& c)
{
    const int aciSayisi = 180;
    int desteklenen = 0;

    int bant = cvRound(c[2] * 0.15);
    if (bant < 2)
        bant = 2;

    for (int t = 0; t < aciSayisi; t++)
    {
        double aci = 2.0 * CV_PI * t / aciSayisi;
        bool kirmiziVar = false;

        for (int dr = -bant; dr <= bant; dr++)
        {
            double r = c[2] + dr;

            if (r <= 0)
                continue;

            int x = cvRound(c[0] + r * cos(aci));
            int y = cvRound(c[1] + r * sin(aci));

            if (x < 0 || y < 0 ||
                x >= maske.cols || y >= maske.rows)
            {
                continue;
            }

            if (maske.at<uchar>(y, x) != 0)
            {
                kirmiziVar = true;
                break;
            }
        }

        if (kirmiziVar)
            desteklenen++;
    }

    return static_cast<double>(desteklenen) / aciSayisi;
}

int main()
{
    try
    {
        vector<String> dosyalar;
        dosyalariBul(dosyalar);

        if (dosyalar.empty())
        {
            cerr << "trafik klasorunde goruntu bulunamadi!" << endl;
            return -1;
        }

        ofstream rapor("odev2_sonuclar.csv");

        if (!rapor)
        {
            cerr << "Rapor olusturulamadi!" << endl;
            return -1;
        }

        rapor << "goruntu_no,rgb_kirmizi,hsv_kirmizi,"
            << "lab_kirmizi,hough_adayi,kabul_edilen\n";

        const double minimumKirmiziOrani = 0.55;
        const double houghOyEsigi = 18.0;

        int islenen = 0;

        for (const String& dosya : dosyalar)
        {
            Mat resim = imread(dosya, IMREAD_COLOR);

            if (resim.empty())
            {
                cout << "Okunamadi: " << dosya << endl;
                continue;
            }

            int kisaKenar = min(resim.cols, resim.rows);
            int uzunKenar = max(resim.cols, resim.rows);

            double olcek = kisaKenar < 100
                ? 100.0 / kisaKenar
                : 1.0;

            if (uzunKenar * olcek > 800)
                olcek = 800.0 / uzunKenar;

            if (abs(olcek - 1.0) > 0.001)
            {
                Mat boyutlandirilmis;

                resize(
                    resim, boyutlandirilmis,
                    Size(), olcek, olcek,
                    olcek < 1.0 ? INTER_AREA : INTER_CUBIC
                );

                resim = boyutlandirilmis;
            }

            Mat rgbMaske, hsvMaske, labMaske, ortakMaske;

            kirmiziMaskeleri(
                resim,
                rgbMaske,
                hsvMaske,
                labMaske,
                ortakMaske
            );

            Mat gri;
            cvtColor(resim, gri, COLOR_BGR2GRAY);
            GaussianBlur(gri, gri, Size(5, 5), 1.2);

            vector<Vec3f> adaylar;

            int maksimumYaricap =
                min(resim.cols, resim.rows) / 2;

            double minimumMerkezMesafesi =
                max(12.0, min(resim.cols, resim.rows) / 8.0);

            HoughCircles(
                gri,
                adaylar,
                HOUGH_GRADIENT,
                1.0,
                minimumMerkezMesafesi,
                100.0,
                houghOyEsigi,
                4,
                maksimumYaricap
            );

            Mat sonuc = resim.clone();
            int kabulEdilen = 0;

            for (const Vec3f& c : adaylar)
            {
                double oran = cevreKirmiziOrani(ortakMaske, c);

                if (oran < minimumKirmiziOrani)
                    continue;

                Point merkez(cvRound(c[0]), cvRound(c[1]));
                int yaricap = cvRound(c[2]);

                circle(
                    sonuc, merkez, yaricap,
                    Scalar(0, 255, 0), 2, LINE_AA
                );

                circle(
                    sonuc, merkez, 2,
                    Scalar(255, 0, 0), FILLED
                );

                putText(
                    sonuc,
                    format("Kirmizi: %.0f%%", oran * 100.0),
                    Point(
                        max(0, merkez.x - yaricap),
                        max(15, merkez.y - yaricap - 5)
                    ),
                    FONT_HERSHEY_SIMPLEX,
                    0.4,
                    Scalar(0, 200, 0),
                    1
                );

                kabulEdilen++;
            }

            string onEk = "odev2_" + to_string(++islenen);

            bool kayit = true;

            kayit &= imwrite(onEk + "_girdi.png", resim);
            kayit &= imwrite(onEk + "_rgb_maske.png", rgbMaske);
            kayit &= imwrite(onEk + "_hsv_maske.png", hsvMaske);
            kayit &= imwrite(onEk + "_lab_maske.png", labMaske);
            kayit &= imwrite(onEk + "_ortak_maske.png", ortakMaske);
            kayit &= imwrite(onEk + "_sonuc.png", sonuc);

            cout << "\nGoruntu " << islenen << ": " << dosya << endl;
            cout << "Hough adayi: " << adaylar.size()
                << " | Kabul edilen: " << kabulEdilen << endl;

            if (!kayit)
                cerr << "Bazi ciktilar kaydedilemedi!" << endl;

            rapor << islenen << ","
                << countNonZero(rgbMaske) << ","
                << countNonZero(hsvMaske) << ","
                << countNonZero(labMaske) << ","
                << adaylar.size() << ","
                << kabulEdilen << "\n";

            namedWindow("Girdi", WINDOW_NORMAL);
            namedWindow("Kirmizi Maske", WINDOW_NORMAL);
            namedWindow("Tespit Sonucu", WINDOW_NORMAL);

            imshow("Girdi", resim);
            imshow("Kirmizi Maske", ortakMaske);
            imshow("Tespit Sonucu", sonuc);

            int tus = waitKey(200);

            if (tus == 27)
                break;
        }

        rapor.close();

        ofstream aciklama("odev2_aciklama.txt");

        aciklama
            << "RGB, HSV ve Lab uzaylarinda kirmizi maskeleri "
            << "olusturulmustur.\n"
            << "En az iki yontemin destekledigi pikseller "
            << "ortak maskeye alinmistir.\n"
            << "Gri goruntude HoughCircles ile cember adaylari "
            << "bulunmustur.\n"
            << "Cevresindeki acilarin en az %55'inde kirmizi "
            << "destegi olan adaylar kabul edilmistir.\n"
            << "Sonuclardaki yesil cemberler kabul edilen adaylardir.\n\n"
            << "Esikler baslangic degerleridir; veriyle "
            << "test edilip ayarlanmalidir.\n"
            << "Egik, kucuk, bulanik veya kismen kapali "
            << "isaretler kacirilabilir.\n"
            << "Kirmizi dairesel baska nesneler yanlis "
            << "tespit edilebilir.\n"
            << "Bu sistem isaretin anlamini siniflandirmaz.\n"
            << "Kucuk goruntuleri buyutmek yeni ayrinti eklemez.\n"
            << "Raporlanan sayilar dogruluk orani degildir; "
            << "dogruluk icin etiketli referans gerekir.\n";

        aciklama.close();

        cout << "\nIslenen goruntu: " << islenen << endl;
        cout << "Cikmak icin bir goruntu penceresinde tusa basin."
            << endl;

        if (islenen > 0)
            waitKey(0);

        destroyAllWindows();
    }
    catch (const exception& hata)
    {
        cerr << "Hata: " << hata.what() << endl;
        return -1;
    }

    return 0;
}