#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <stdexcept>

using namespace cv;
using namespace std;

int main()
{
    try
    {
        const int genislik = 7680;
        const int yukseklik = 4320;

        // OpenCV kanal sirasi: B, G, R.
        {
            Mat uretilen(yukseklik, genislik, CV_8UC3);

            for (int y = 0; y < yukseklik; y++)
            {
                Vec3b* satir = uretilen.ptr<Vec3b>(y);

                for (int x = 0; x < genislik; x++)
                {
                    satir[x][0] = static_cast<uchar>(
                        255LL * x / (genislik - 1)
                        );

                    satir[x][1] = static_cast<uchar>(
                        255LL * y / (yukseklik - 1)
                        );

                    satir[x][2] = static_cast<uchar>(
                        255LL * (x + y) /
                        (genislik + yukseklik - 2)
                        );
                }
            }

            if (!imwrite("gorev3_8k_orijinal.bmp", uretilen))
                throw runtime_error("Orijinal BMP kaydedilemedi.");
        }

        Mat resim = imread(
            "gorev3_8k_orijinal.bmp",
            IMREAD_COLOR
        );

        if (resim.empty())
            throw runtime_error("8K BMP acilamadi.");

        if (resim.cols != genislik ||
            resim.rows != yukseklik ||
            resim.type() != CV_8UC3)
        {
            throw runtime_error("Goruntu boyutu veya tipi hatali.");
        }

        cout << "Cozunurluk: "
            << resim.cols << " x " << resim.rows << endl;

        cout << "Kanal sayisi: " << resim.channels() << endl;
        cout << "Piksel sayisi: " << resim.total() << endl;

        // Sonradan gorsel karsilastirma icin kucuk onizleme.
        // Bu islem olcumun DISINDA.
        Mat onceOnizleme;
        resize(
            resim,
            onceOnizleme,
            Size(960, 540),
            0, 0,
            INTER_AREA
        );

        Vec3b ornekOnce = resim.at<Vec3b>(
            yukseklik / 2,
            genislik / 2
        );

        // steady_clock, gecen sure olcumu icin uygundur.
        const auto baslangic = chrono::steady_clock::now();

        for (int y = 0; y < resim.rows; y++)
        {
            Vec3b* satir = resim.ptr<Vec3b>(y);

            for (int x = 0; x < resim.cols; x++)
            {
                satir[x][0] = satir[x][0] / 2;
                satir[x][1] = satir[x][1] / 2;
                satir[x][2] = satir[x][2] / 2;
            }
        }

        const auto bitis = chrono::steady_clock::now();

        double milisaniye =
            chrono::duration<double, milli>(
                bitis - baslangic
            ).count();

        double mikrosaniye =
            chrono::duration<double, micro>(
                bitis - baslangic
            ).count();

        cout << fixed << setprecision(3);
        cout << "\nYalniz duzenleme suresi: "
            << milisaniye << " ms" << endl;

        cout << "Yalniz duzenleme suresi: "
            << mikrosaniye << " us" << endl;

        if (!imwrite("gorev3_8k_duzenlenmis.bmp", resim))
            throw runtime_error("Duzenlenmis BMP kaydedilemedi.");

        Vec3b ornekSonra = resim.at<Vec3b>(
            yukseklik / 2,
            genislik / 2
        );

        bool kontrol =
            ornekSonra[0] == ornekOnce[0] / 2 &&
            ornekSonra[1] == ornekOnce[1] / 2 &&
            ornekSonra[2] == ornekOnce[2] / 2;

        // 5. Sonuclari raporla.
        ofstream rapor("gorev3_sure_raporu.txt");

        if (!rapor)
            throw runtime_error("Rapor olusturulamadi.");

        rapor << fixed << setprecision(6);

        rapor << "GOREV 3 - 8K GORUNTU DUZENLEME\n\n";
        rapor << "Cozunurluk: " << genislik
            << " x " << yukseklik << "\n";
        rapor << "Kanal sayisi: 3\n";
        rapor << "Kanal basina bit: 8\n";
        rapor << "Piksel sayisi: " << resim.total() << "\n";

        rapor << "Ham piksel verisi: "
            << static_cast<long long>(genislik) *
            yukseklik * 3
            << " byte\n\n";

        rapor << "Islem: Her B, G ve R degerini 2'ye bolme.\n";
        rapor << "Yontem: Tek is parcacikli piksel dongusu.\n";
        rapor << "Olcum: std::chrono::steady_clock\n";

        rapor << "Duzenleme suresi: "
            << milisaniye << " ms\n";

        rapor << "Duzenleme suresi: "
            << mikrosaniye << " us\n\n";

        rapor << "Ornek piksel once (B,G,R): "
            << static_cast<int>(ornekOnce[0]) << ","
            << static_cast<int>(ornekOnce[1]) << ","
            << static_cast<int>(ornekOnce[2]) << "\n";

        rapor << "Ornek piksel sonra (B,G,R): "
            << static_cast<int>(ornekSonra[0]) << ","
            << static_cast<int>(ornekSonra[1]) << ","
            << static_cast<int>(ornekSonra[2]) << "\n";

        rapor << "Ornek piksel kontrolu: "
            << (kontrol ? "BASARILI" : "BASARISIZ") << "\n\n";

        rapor
            << "Acma, kaydetme, goruntu olusturma ve onizleme "
            << "islemleri olcumun disindadir.\n"
            << "Tamsayi bolmesi kullanildigi icin tek sayili "
            << "kanal degerlerinde kesir atilir. Ornek: 255/2=127.\n"
            << "Kanal degerleri azalir; sonuc goruntu daha koyu "
            << "gorunur. Algisal parlakligin tam yariya "
            << "indigi anlami cikartilmamalidir.\n"
            << "Raporlanan sure tek calistirmanin sonucudur; "
            << "donanim, derleme ayarlari ve sistem yukune "
            << "gore degisebilir.\n";

        rapor.close();

        Mat sonraOnizleme;

        resize(
            resim,
            sonraOnizleme,
            Size(960, 540),
            0, 0,
            INTER_AREA
        );

        imshow("8K Orijinal - Kucuk Onizleme", onceOnizleme);
        imshow("8K Ikiye Bolunmus - Kucuk Onizleme", sonraOnizleme);

        waitKey(0);
    }
    catch (const exception& hata)
    {
        cerr << "Hata: " << hata.what() << endl;
        return -1;
    }

    return 0;
}