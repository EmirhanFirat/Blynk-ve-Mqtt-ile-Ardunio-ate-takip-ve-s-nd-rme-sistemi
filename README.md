# 🚒 OrmanGözü: IoT Tabanlı Akıllı İtfaiye ve Erken Uyarı Sistemi

Bu proje, orman yangınlarının erken tespiti veya çiftçilerin anız yakarken tarlalarını yakmamaları için geliştirilen  ve otomatik müdahale için geliştirilmiş bir IoT (Nesnelerin İnterneti) prototipidir. Sistem, sensör verilerini MQTT protokolü ile anlık olarak işler ve Python tabanlı bir "Büyük Veri" paneli (Dashboard) üzerinden analiz eder.

## 🌟 Öne Çıkan Özellikler

* **Otomatik Yangın Algılama:** Alev sensörleri ile yangının yönünü (Sol/Sağ/Orta) tespit eder.
* **Otonom Müdahale:** Servo motor ve su pompası ile hedef odaklı söndürme yapar.
* **Canlı İzleme (IoT):** Blynk mobil uygulaması ve Python Dashboard üzerinden anlık su seviyesi ve sistem durumu takibi.
* **Büyük Veri Analitiği:** Sensör verilerini simüle ederek ve kaydederek (.csv) geçmişe dönük risk analizi ve raporlama yapar.
* **Hibrit Mimari:** NodeMCU (Uç Birim) + Python (Veri Merkezi) entegrasyonu.

## 🛠️ Kullanılan Teknolojiler

### Donanım
* NodeMCU (ESP8266)
* HC-SR04 Mesafe Sensörü (Su seviyesi ölçümü için)
* 3x Alev Sensörü (IR Flame Sensor)
* Servo Motor & Mini Su Pompası
* OLED Ekran & Buzzer
*Ardunio Uno

### Yazılım & Protokoller
* **Gömülü Yazılım:** C++ (Arduino IDE)
* **Veri Analizi & Görselleştirme:** Python (Matplotlib, Pandas, Paho-MQTT)
* **Haberleşme:** MQTT (Mosquitto Broker), WiFi
* **Mobil Arayüz:** Blynk IoT

## 📊 Proje Mimarisi
Sistem, sahadaki sensör verilerini `test.mosquitto.org` broker'ı üzerinden Python scriptine gönderir. Python kodu bu veriyi işler, Excel dosyasına kaydeder ve canlı grafikler oluşturur.

## 👥 Proje Ekibi

* **Emirhan Fırat:** Yazılım Geliştirme (Python/Arduino), Veri Analizi, Dashboard Tasarımı.

---
*Bu proje, Tarım ve Orman Bakanlığı envanterine uygun bir prototip olarak geliştirilmiştir.*
