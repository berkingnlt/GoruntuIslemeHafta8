#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <stdexcept>

using namespace cv;
using namespace std;

long long dosyaBoyutu(const string& dosyaAdi)
{
    ifstream dosya(dosyaAdi, ios::binary | ios::ate);

    if (!dosya)
        throw runtime_error("Dosya acilamadi: " + dosyaAdi);

    return static_cast<long long>(dosya.tellg());
}

void bilgileriYaz(
    const string& baslik,
    const Mat& resim,
    const string& dosyaAdi,
    ofstream& rapor)
{
    long long hamVeri =
        static_cast<long long>(resim.rows) *
        resim.cols * resim.channels();

    long long satirByte =
        static_cast<long long>(resim.cols) * resim.channels();

    long long hizaliSatirByte =
        ((satirByte + 3) / 4) * 4;

    long long bmpPikselVerisi =
        hizaliSatirByte * resim.rows;

    long long gercekBoyut = dosyaBoyutu(dosyaAdi);

    auto yaz = [&](ostream& cikti)
        {
            cikti << "\n--- " << baslik << " ---\n";
            cikti << "Dosya: " << dosyaAdi << "\n";
            cikti << "Cozunurluk: "
                << resim.cols << " x " << resim.rows << "\n";
            cikti << "Kanal sayisi: " << resim.channels() << "\n";
            cikti << "Kanal basina bit: 8\n";
            cikti << "Piksel basina bit: "
                << resim.channels() * 8 << "\n";

            cikti << "Ham piksel verisi: "
                << hamVeri << " byte\n";

            cikti << "Satir hizalamali BMP piksel verisi: "
                << bmpPikselVerisi << " byte\n";

            cikti << "Gercek dosya boyutu: "
                << gercekBoyut << " byte\n";

            cikti << fixed << setprecision(4);
            cikti << "Gercek dosya boyutu: "
                << gercekBoyut / (1024.0 * 1024.0)
                << " MiB\n";

            cikti << "Piksel verisi disindaki dosya alani: "
                << gercekBoyut - bmpPikselVerisi
                << " byte\n";
        };

    yaz(cout);
    yaz(rapor);
}

int main()
{
    try
    {
        Mat kaynak = imread("insanlar.jpeg", IMREAD_COLOR);

        if (kaynak.empty())
        {
            throw runtime_error(
                "insanlar.jpeg bulunamadi! "
                "Fotografi proje klasorune koy."
            );
        }

        Mat hazirlanan;

        resize(
            kaynak,
            hazirlanan,
            Size(1000, 1000),
            0, 0,
            INTER_AREA
        );

        if (!imwrite("gorev2_renkli_1000.bmp", hazirlanan))
            throw runtime_error("Renkli BMP kaydedilemedi.");

        // 2. Gorevin istedigi gibi BMP dosyasini diskten ac.
        Mat renkli = imread(
            "gorev2_renkli_1000.bmp",
            IMREAD_COLOR
        );

        if (renkli.empty())
            throw runtime_error("Renkli BMP acilamadi.");

        // 3. Tek kanalli gri goruntuye donustur.
        Mat gri;
        cvtColor(renkli, gri, COLOR_BGR2GRAY);

        // 4. BMP olarak kaydet.
        if (!imwrite("gorev2_gri_1000.bmp", gri))
            throw runtime_error("Gri BMP kaydedilemedi.");

        // 5. Kaydedilen gri dosyayi kanal sayisini
        // degistirmeden yeniden ac.
        Mat kaydedilenGri = imread(
            "gorev2_gri_1000.bmp",
            IMREAD_UNCHANGED
        );

        if (kaydedilenGri.empty())
            throw runtime_error("Gri BMP acilamadi.");

        // 6. Disk boyutlarini raporla.
        ofstream rapor("gorev2_boyut_raporu.txt");

        if (!rapor)
            throw runtime_error("Rapor olusturulamadi.");

        bilgileriYaz(
            "RENKLI GORUNTU",
            renkli,
            "gorev2_renkli_1000.bmp",
            rapor
        );

        bilgileriYaz(
            "GRI GORUNTU",
            kaydedilenGri,
            "gorev2_gri_1000.bmp",
            rapor
        );

        long long renkliBoyut =
            dosyaBoyutu("gorev2_renkli_1000.bmp");

        long long griBoyut =
            dosyaBoyutu("gorev2_gri_1000.bmp");

        double oran =
            static_cast<double>(renkliBoyut) / griBoyut;

        cout << "\nRenkli / gri dosya boyutu orani: "
            << oran << endl;

        rapor << "\nRenkli / gri dosya boyutu orani: "
            << oran << "\n\n";

        rapor
            << "YORUM:\n"
            << "Iki goruntunun cozunurlugu aynidir: 1000x1000.\n"
            << "Renkli goruntu 3 kanal, gri goruntu 1 kanaldir.\n"
            << "Her kanal 8 bit oldugundan ham veri boyutlari:\n"
            << "Renkli: 1000*1000*3 = 3000000 byte.\n"
            << "Gri: 1000*1000*1 = 1000000 byte.\n"
            << "Ham veri boyutlari arasindaki oran tam 3'tur.\n"
            << "BMP dosyalarinda basliklar, gri BMP'de renk "
            << "tablosu ve gerektiginde satir hizalama baytlari "
            << "da bulunabilir.\n"
            << "Bu nedenle gercek dosya boyutu ham piksel "
            << "verisinden buyuktur; dosya boyutu orani "
            << "tam olarak 3 olmayabilir.\n"
            << "Bu boyutta iki goruntunun satir uzunluklari "
            << "4'un kati oldugu icin satir dolgusu gerekmez.\n";

        rapor.close();

        // 7. Goruntuleri goster.
        namedWindow("1000x1000 Renkli BMP", WINDOW_NORMAL);
        namedWindow("1000x1000 Gri BMP", WINDOW_NORMAL);

        imshow("1000x1000 Renkli BMP", renkli);
        imshow("1000x1000 Gri BMP", kaydedilenGri);

        waitKey(0);
    }
    catch (const exception& hata)
    {
        cerr << "Hata: " << hata.what() << endl;
        return -1;
    }

    return 0;
}