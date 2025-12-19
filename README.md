# 🚒 OrmanGözü: IoT Tabanlı Akıllı İtfaiye ve Erken Uyarı Sistemi

Bu proje, orman yangınlarının veya tarımsal alanlardaki anız yangınlarının erken tespiti ve otonom müdahalesi için geliştirilmiş bir IoT (Nesnelerin İnterneti) prototipidir. Sistem, NodeMCU tabanlı bir istasyonun sensör verilerini MQTT protokolü ile anlık olarak bir veri merkezine iletir. Python tabanlı bir dashboard, bu verileri işleyerek hem canlı izleme hem de geçmişe dönük "Büyük Veri" analizi imkanı sunar.

## 🌟 Öne Çıkan Özellikler

*   **Otomatik Yangın Algılama:** 3 adet alev sensörü ile yangının yönünü (Sol/Sağ/Orta) hassas bir şekilde tespit eder.
*   **Otonom Müdahale:** Algılanan yangın yönüne servo motor ile dönerek mini su pompası ile hedef odaklı söndürme yapar.
*   **Hibrit Kontrol:** Sistem hem tam otonom çalışabilir hem de Blynk mobil uygulaması üzerinden manuel olarak kontrol edilebilir.
*   **Canlı İzleme:** Blynk uygulaması ve Python dashboard üzerinden anlık su seviyesi, yangın durumu ve sistem modu (manuel/otomatik) takibi.
*   **Gelişmiş Veri Analitiği:** Python scripti, 7 günlük sahte sensör verisi üreterek `akilli_istasyon_verileri.csv` dosyasına kaydeder. Canlı veriler de anlık olarak bu dosyaya eklenir, böylece büyük veri analizi ve raporlama için kalıcı bir veri seti oluşturulur.

## 🛠️ Mimarisi ve Çalışma Prensibi

Sistem iki ana bileşenden oluşur:

1.  **Donanım (NodeMCU):** Sahadaki istasyon, alev ve mesafe sensörlerinden gelen verileri okur. Yangın algılandığında otonom olarak müdahale eder. Tüm sensör verilerini (Su Seviyesi, Yangın Durumu, Pompa Durumu) JSON formatında paketleyerek `test.mosquitto.org` adresindeki halka açık MQTT broker'ına yayınlar. Aynı zamanda Blynk sunucusu ile haberleşerek mobil kontrol ve izleme sağlar.
2.  **Yazılım (Python Dashboard):** `dashboard.py` script'i, MQTT broker'ına bağlanarak sensör verilerini dinler. Gelen canlı verileri `akilli_istasyon_verileri.csv` dosyasına ekler. Başlangıçta, geçmişe dönük analiz için büyük bir veri seti simüle eder. Matplotlib kullanarak geçmiş verilerin analizini ve canlı sistem durumunu gösteren bir arayüz sunar.

## 💻 Kullanılan Teknolojiler

### Donanım
*   NodeMCU (ESP8266)
*   Arduino Uno (Prototipleme)
*   3x Alev Sensörü (IR Flame Sensor)
*   HC-SR04 Ultrasonik Mesafe Sensörü (Su seviyesi ölçümü için)
*   SG90 Servo Motor
*   Mini Su Pompası
*   16x2 I2C LCD Ekran

### Yazılım & Protokoller
*   **Gömülü Yazılım:** C++ (Arduino IDE)
*   **Veri Analizi & Görselleştirme:** Python (Pandas, Matplotlib, NumPy)
*   **Haberleşme & IoT:** MQTT (Paho-MQTT), Blynk
*   **Veritabanı:** CSV Dosyası (`akilli_istasyon_verileri.csv`)

## 🚀 Kurulum ve Çalıştırma

### 1. Donanım Bağlantıları
Sensör ve aktüatörleri `Ardunio/sketch_oct23a/sketch_oct23a.ino` dosyasında belirtilen pinlere göre NodeMCU'ya bağlayın:
*   I2C LCD: `D1` (SCL), `D2` (SDA)
*   Alev Sensörleri: `D0` (Sol), `D3` (Sağ), `D4` (Orta)
*   Servo Motor: `D5`
*   Su Pompası: `D6`
*   HC-SR04: `D7` (Trig), `D8` (Echo)

### 2. Arduino (NodeMCU) Firmware Yüklemesi
1.  Arduino IDE'yi açın ve ESP8266 kart yöneticisini kurun.
2.  Gerekli kütüphaneleri yükleyin: `Blynk`, `PubSubClient`, `Servo`, `LiquidCrystal_I2C`.
3.  `Ardunio/sketch_oct23a/sketch_oct23a.ino` dosyasını açın.
4.  Dosya içerisindeki aşağıdaki alanları kendi bilgilerinizle güncelleyin:
    ```cpp
    // Blynk Konsol Bilgileri
    #define BLYNK_TEMPLATE_ID "SENIN_TEMPLATE_ID"
    #define BLYNK_TEMPLATE_NAME "SENIN_TEMPLATE_NAME"
    #define BLYNK_AUTH_TOKEN "SENIN_BLYNK_TOKEN"

    // WiFi Bilgileri
    char ssid[] = "SENIN_WIFI_ADIN";
    char pass[] = "SENIN_WIFI_SIFREN";
    ```
5.  Kodu NodeMCU kartınıza yükleyin.

### 3. Python Dashboard'u Çalıştırma
1.  Bilgisayarınızda Python 3 kurulu olduğundan emin olun.
2.  Gerekli Python kütüphanelerini yükleyin:
    ```bash
    pip install paho-mqtt matplotlib pandas numpy
    ```
3.  `dashboard.py` dosyasını çalıştırın:
    ```bash
    python dashboard.py
    ```
    Script ilk çalıştığında 7 günlük simülasyon verisi oluşturup `akilli_istasyon_verileri.csv` dosyasına kaydedecek ve ardından canlı verileri göstermeye başlayacaktır.

## 📂 Dosya Yapısı

```
.
├── Ardunio/
│   └── sketch_oct23a/
│       └── sketch_oct23a.ino      # NodeMCU için ana firmware kodu
├── README.md                      # Bu döküman
├── akilli_istasyon_verileri.csv   # Hem simülasyon hem de canlı verilerin kaydedildiği CSV dosyası
└── dashboard.py                   # MQTT verilerini dinleyen ve görselleştiren Python scripti
```

## 👥 Proje Ekibi

*   **Emirhan Fırat:** Yazılım Geliştirme (Python/Arduino), Veri Analizi, Dashboard Tasarımı.
