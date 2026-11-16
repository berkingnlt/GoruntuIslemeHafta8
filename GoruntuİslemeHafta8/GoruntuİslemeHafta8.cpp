#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <fstream>

using namespace cv;
using namespace std;

int main()
{
    VideoCapture kamera(0);

    if (!kamera.isOpened())
    {
        cerr << "Web kamerasi acilamadi!" << endl;
        return -1;
    }

    const int farkEsigi = 25;
    const int minimumHareketPikseli = 500;
    const double minimumBolgeAlani = 150.0;

    Mat oncekiGri;
    Mat cekirdek = getStructuringElement(
        MORPH_RECT, Size(3, 3)
    );

    int kayitSayisi = 0;

    cout << "S: Goruntuleri kaydet\n";
    cout << "Q veya ESC: Cikis\n";

    while (true)
    {
        Mat kare;

        if (!kamera.read(kare) || kare.empty())
        {
            cerr << "Kameradan goruntu alinamadi." << endl;
            break;
        }

        resize(kare, kare, Size(640, 480));

        Mat gri;
        cvtColor(kare, gri, COLOR_BGR2GRAY);
        GaussianBlur(gri, gri, Size(5, 5), 0);

        if (oncekiGri.empty())
        {
            oncekiGri = gri.clone();
            continue;
        }

        Mat fark, hareketMaskesi;
        absdiff(gri, oncekiGri, fark);
        threshold(fark, hareketMaskesi, farkEsigi, 255, THRESH_BINARY);

        morphologyEx(
            hareketMaskesi, hareketMaskesi,
            MORPH_OPEN, cekirdek
        );

        morphologyEx(
            hareketMaskesi, hareketMaskesi,
            MORPH_CLOSE, cekirdek
        );

        int hareketPikseli = countNonZero(hareketMaskesi);

        Mat sonuc = kare.clone();
        Mat tenMaskesi = Mat::zeros(kare.size(), CV_8UC1);

        int roiSayisi = 0;

        if (hareketPikseli >= minimumHareketPikseli)
        {
            vector<vector<Point>> konturlar;

            Mat konturGirdisi = hareketMaskesi.clone();

            findContours(
                konturGirdisi,
                konturlar,
                RETR_EXTERNAL,
                CHAIN_APPROX_SIMPLE
            );

            for (const auto& kontur : konturlar)
            {
                if (kontur.size() < 3)
                    continue;

                int sol = kontur[0].x;
                int sag = kontur[0].x;
                int ust = kontur[0].y;
                int alt = kontur[0].y;

                double ikiAlan = 0.0;

                for (size_t i = 0; i < kontur.size(); i++)
                {
                    Point p = kontur[i];
                    Point sonraki = kontur[(i + 1) % kontur.size()];

                    ikiAlan +=
                        static_cast<double>(p.x) * sonraki.y -
                        static_cast<double>(sonraki.x) * p.y;

                    if (p.x < sol) sol = p.x;
                    if (p.x > sag) sag = p.x;
                    if (p.y < ust) ust = p.y;
                    if (p.y > alt) alt = p.y;
                }

                double alan = (ikiAlan < 0 ? -ikiAlan : ikiAlan) / 2.0;

                if (alan < minimumBolgeAlani)
                    continue;

                Rect roi(sol, ust, sag - sol + 1, alt - ust + 1);

                Mat ycrcb, yerelTenMaskesi;

                cvtColor(kare(roi), ycrcb, COLOR_BGR2YCrCb);

                inRange(
                    ycrcb,
                    Scalar(0, 133, 77),
                    Scalar(255, 173, 127),
                    yerelTenMaskesi
                );

                bitwise_and(
                    yerelTenMaskesi,
                    hareketMaskesi(roi),
                    yerelTenMaskesi
                );

                Mat hedefMaske = tenMaskesi(roi);

                bitwise_or(
                    hedefMaske,
                    yerelTenMaskesi,
                    hedefMaske
                );

                rectangle(sonuc, roi, Scalar(0, 255, 255), 2);

                roiSayisi++;
            }
        }

        sonuc.setTo(Scalar(0, 255, 0), tenMaskesi);

        putText(
            sonuc,
            "Hareket: " + to_string(hareketPikseli) +
            "  ROI: " + to_string(roiSayisi),
            Point(10, 25),
            FONT_HERSHEY_SIMPLEX,
            0.6,
            Scalar(255, 255, 255),
            2
        );

        putText(
            sonuc,
            "Ten adayi: " + to_string(countNonZero(tenMaskesi)),
            Point(10, 50),
            FONT_HERSHEY_SIMPLEX,
            0.6,
            Scalar(255, 255, 255),
            2
        );

        imshow("Orijinal", kare);
        imshow("Hareket Maskesi", hareketMaskesi);
        imshow("Hareketli ROI - Ten Tespiti", sonuc);
        imshow("Hareketli Ten Maskesi", tenMaskesi);

        oncekiGri = gri.clone();

        int tus = waitKey(1);

        if (tus == 27 || tus == 'q' || tus == 'Q')
            break;

        if (tus == 's' || tus == 'S')
        {
            string onEk =
                "odev1_kayit_" + to_string(++kayitSayisi);

            bool basarili = true;

            basarili &= imwrite(onEk + "_orijinal.bmp", kare);
            basarili &= imwrite(onEk + "_hareket.bmp", hareketMaskesi);
            basarili &= imwrite(onEk + "_ten_maskesi.bmp", tenMaskesi);
            basarili &= imwrite(onEk + "_sonuc.bmp", sonuc);

            cout << (basarili ? "Kaydedildi: " : "Kayit hatasi: ")
                << onEk << endl;
        }
    }

    kamera.release();
    destroyAllWindows();

    ofstream rapor("odev1_aciklama.txt");

    rapor
        << "Ardisik kareler griye donusturulup yumusatilmistir.\n"
        << "Mutlak kare farki esiklenerek hareket maskesi elde edilmistir.\n"
        << "Kucuk gurultuler morfolojik islemlerle azaltilmistir.\n"
        << "Yeterli hareket varsa hareket bolgelerinden ROI secilmistir.\n"
        << "YCbCr donusumu ve ten esiklemesi yalniz ROI icinde yapilmistir.\n"
        << "Hareketli ve ten adayi pikseller yesile boyanmistir.\n"
        << "Diger pikseller arka plan kabul edilip korunmustur.\n\n"
        << "Fark esigi: " << farkEsigi << "\n"
        << "Minimum hareket pikseli: " << minimumHareketPikseli << "\n"
        << "Minimum kontur alani: " << minimumBolgeAlani << "\n"
        << "Ten esikleri: 133<=Cr<=173, 77<=Cb<=127.\n\n"
        << "Sinirlamalar:\n"
        << "Kamera hareketi ve ani isik degisimi hareket sanilabilir.\n"
        << "Hareketsiz kalan kisi arka plan kabul edilir.\n"
        << "Ten esikleri her ten tonu ve isik kosulunda basarili degildir.\n"
        << "Bu uygulama kisi tanima degil, hareketli ten adayi tespitidir.\n";

    return 0;
}