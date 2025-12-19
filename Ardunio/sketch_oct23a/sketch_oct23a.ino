// ================= BLYNK AYARLARI (EN ÜSTE EKLENDİ) =================
/* Lütfen Blynk Konsolundan aldığın 3 satırı buraya yapıştır */
#define BLYNK_TEMPLATE_ID " Oluşturulan Blynk Id giriniz"
#define BLYNK_TEMPLATE_NAME "Oluşturulan Blynk Template'in ismin giriniz"
#define BLYNK_AUTH_TOKEN "Oluşturulan Blynk token giriniz"

#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h> // Blynk Kütüphanesi Eklendi
#include <PubSubClient.h>       // MQTT Kütüphanesi
#include <Servo.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// ================= AYARLAR =================
// 1. WiFi Bilgileri
char ssid[] = "Wifi bilgisi giriniz";
char pass[] = "wifi şifresi giriniz";

// 2. MQTT Bilgileri (Halka açık sunucu)
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* mqtt_topic = "okulproje/istasyon/veri"; // Python buradan dinleyecek

// 3. Pin Tanımlamaları
#define SCL_PIN     D1 
#define SDA_PIN     D2  
#define SENSOR_SOL  D0  
#define SENSOR_SAG  D3  
#define SENSOR_ORTA D4  
#define SERVO_PIN   D5  
#define POMPA_PIN   D6  
#define TRIG_PIN    D7  
#define ECHO_PIN    D8  

// 4. Tank Ayarları (Peynir Kabı: 12cm)
const float KAP_YUKSEKLIK_CM = 12.0;
const float MIN_OLCUM_CM = 2.0;       
const float SENSOR_OFFSET_CM = 0.0;   

const int SAMPLE_COUNT = 9; // Filtre için örnek sayısı

// 1 cm lookup tablosu (0..12 cm)
const int percentTable[13] = {
  0,  8, 17, 25, 33, 42, 50, 58, 67, 75, 83, 92, 100
};

// ================= NESNELER =================
Servo myServo; 
LiquidCrystal_I2C lcd(0x27, 16, 2); 
WiFiClient espClient;
PubSubClient mqttClient(espClient);
BlynkTimer timer; // Blynk zamanlayıcısı

// ================= DEĞİŞKENLER =================
int suSeviyesiYuzde = 0;
float suYukseklikCm = 0.0;
int suSeviyeCm = 0;
unsigned long lastMsgTime = 0; // Veri gönderme zamanlayıcısı

bool solDurum = false;
bool ortaDurum = false;
bool sagDurum = false;
bool genelYangin = false;

// BLYNK İÇİN YENİ DEĞİŞKENLER
bool manuelMod = false;    // V0 (0: OTO, 1: MANUEL)
int yanginGrafigiVerisi = 0; // V4 (0, 10, 20, 30)

// ----------------- Yardımcı: Sıralama (Filtre için) -----------------
void sortArray(float* arr, int n) {
  for (int i = 0; i < n - 1; i++) {
    for (int j = 0; j < n - 1 - i; j++) {
      if (arr[j] > arr[j + 1]) {
        float t = arr[j];
        arr[j] = arr[j + 1];
        arr[j + 1] = t;
      }
    }
  }
}

// ----------------- Tek Ölçüm -----------------
float readDistanceOnceCm() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL); 
  if (duration == 0) return -1;

  float distance = (duration * 0.0343f) / 2.0f; 
  distance -= SENSOR_OFFSET_CM;
  return distance;
}

// ----------------- Filtreli Ölçüm -----------------
float readDistanceMedianCm() {
  float samples[SAMPLE_COUNT];
  int got = 0;

  for (int i = 0; i < SAMPLE_COUNT; i++) {
    float d = readDistanceOnceCm();
    if (d >= MIN_OLCUM_CM && d <= 200.0f) {
      samples[got++] = d;
    }
    delay(15); // Blynk olduğu için delay'i biraz kıstım
  }

  if (got == 0) return -1;
  sortArray(samples, got);
  return samples[got / 2];
}

// ----------------- MQTT Bağlantı Fonksiyonu -----------------
void reconnect() {
  // MQTT bağlantısı koptuysa tekrar dene (Blocking olmaması için if ile kontrol ediyoruz)
  if (!mqttClient.connected()) {
    Serial.print("MQTT Baglaniyor...");
    String clientId = "NodeMCU-Ogrenci-";
    clientId += String(random(0xffff), HEX);
    
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("Baglandi!");
    } else {
      // Bağlanamazsa Blynk akışını bozmamak için burada bekleme yapmıyoruz
      // Bir sonraki döngüde tekrar deneyecek.
      Serial.print("Hata rc=");
      Serial.print(mqttClient.state());
    }
  }
}

// ================= BLYNK PINLERİ (V0 ve V1) =================

// V0: Mod Seçimi (Switch) -> 0: Otomatik, 1: Manuel
BLYNK_WRITE(V0) {
  int pinValue = param.asInt();
  manuelMod = (pinValue == 1);
  
  if (!manuelMod) {
    // Otomatiğe geçince servoyu ortaya al
    myServo.write(90);
  } else {
    // Manuele geçince pompayı kapat
    digitalWrite(POMPA_PIN, LOW);
  }
}

// V1: Servo Açı (Slider) -> Sadece Manuel Modda Çalışır
BLYNK_WRITE(V1) {
  if (manuelMod) {
    int pos = param.asInt();
    myServo.write(pos);
  }
}

// ================= ANA SİSTEM MANTIĞI =================
// Bu fonksiyon loop yerine timer ile sürekli çağrılacak
void sistemDongusu() {
  // 1) Su seviyesini ölç
  measureWaterLevel();

  // 2) Yangın sensörleri
  solDurum  = !digitalRead(SENSOR_SOL);
  ortaDurum = !digitalRead(SENSOR_ORTA);
  sagDurum  = !digitalRead(SENSOR_SAG);
  genelYangin = (solDurum || ortaDurum || sagDurum);

  // --- V4 GRAFİK MANTIĞI ---
  if (!genelYangin) {
    yanginGrafigiVerisi = 0; // Güvenli (Taban)
  } else {
    if (solDurum) yanginGrafigiVerisi = 10;      // SOL
    else if (ortaDurum) yanginGrafigiVerisi = 20;// ORTA
    else if (sagDurum) yanginGrafigiVerisi = 30; // SAĞ
  }
  // -------------------------

  // 3) Aksiyon (Manuel Mod Kontrolü Eklendi)
  if (!manuelMod) {
    // --- OTOMATİK MOD (Eski Kodun Aynısı) ---
    if (genelYangin) {
      if (ortaDurum) extinguishFire(90);      
      else if (solDurum) extinguishFire(170); // Eski koddaki 170
      else if (sagDurum) extinguishFire(10);  // Eski koddaki 10
    } else {
      digitalWrite(POMPA_PIN, LOW);
    }
  } else {
    // --- MANUEL MOD ---
    digitalWrite(POMPA_PIN, LOW); // Pompayı zorla kapat
    // Servo hareketi V1 fonksiyonundan kontrol ediliyor
  }

  // 4) LCD Güncelleme (KESİNLİKLE DEĞİŞTİRİLMEDİ)
  updateLCD();

  // 5) Blynk Veri Gönderimi (V2, V3, V4)
  Blynk.virtualWrite(V2, suSeviyesiYuzde);         // Su Deposu
  Blynk.virtualWrite(V3, genelYangin ? 255 : 0);   // Yangın Var LED
  Blynk.virtualWrite(V4, yanginGrafigiVerisi);     // Grafik (0,10,20,30)

  // 6) MQTT Veri Gönderimi (Orijinal Mantık)
  unsigned long now = millis();
  if (now - lastMsgTime > 2000) {
    lastMsgTime = now;
    
    // MQTT bağlı değilse tekrar dene
    if (!mqttClient.connected()) {
      reconnect();
    }

    // JSON Paketi Hazırla: {"su": 83, "yangin": 0, "pompa": 0}
    String jsonPaket = "{";
    jsonPaket += "\"su\":";
    jsonPaket += String(suSeviyesiYuzde);
    jsonPaket += ", \"yangin\":";
    jsonPaket += String(genelYangin ? 1 : 0);
    jsonPaket += ", \"pompa\":";
    jsonPaket += String(digitalRead(POMPA_PIN));
    // İstersen buraya manuel mod bilgisini de ekleyebilirsin ama orijinali bozmamak için eklemedim
    jsonPaket += "}";

    // Veriyi Yayınla
    // Serial.print("Yayinlaniyor: "); // Seri portu çok meşgul etmemek için yorum satırı yaptım
    // Serial.println(jsonPaket);
    mqttClient.publish(mqtt_topic, jsonPaket.c_str());
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  // Pin Modları
  pinMode(SENSOR_SOL, INPUT);
  pinMode(SENSOR_ORTA, INPUT);
  pinMode(SENSOR_SAG, INPUT);
  pinMode(POMPA_PIN, OUTPUT);
  digitalWrite(POMPA_PIN, LOW);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  myServo.attach(SERVO_PIN);
  myServo.write(90); 

  Wire.begin(SDA_PIN, SCL_PIN); 
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Baglaniyor");

  // BLYNK ve WiFi Başlatma (Blynk.begin WiFi'yi de halleder)
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  lcd.setCursor(0, 1);
  lcd.print("IP Aldim!");
  delay(1000); lcd.clear();

  // MQTT Ayarları
  mqttClient.setServer(mqtt_server, mqtt_port);

  // Zamanlayıcıyı kur (Her 500ms'de bir sistemDongusu çalışsın)
  // Bu sayede loop içinde delay kullanmadan sistemi yönetiriz.
  timer.setInterval(500L, sistemDongusu);
}

// ================= LOOP =================
void loop() {
  Blynk.run();      // Blynk sunucusuyla konuş
  mqttClient.loop(); // MQTT sunucusuyla konuş
  timer.run();      // Zamanlanmış görevleri yap (sistemDongusu)
}

// --- FONKSİYONLAR (Orijinal Koddan Aynen Alındı) ---

void measureWaterLevel() {
  float distance = readDistanceMedianCm();
  if (distance < 0) return; 

  if (distance < MIN_OLCUM_CM) distance = MIN_OLCUM_CM;
  if (distance > KAP_YUKSEKLIK_CM) distance = KAP_YUKSEKLIK_CM;

  suYukseklikCm = KAP_YUKSEKLIK_CM - distance;
  suSeviyeCm = (int)(suYukseklikCm + 0.5f); // Yuvarlama

  if (suSeviyeCm < 0) suSeviyeCm = 0;
  if (suSeviyeCm > 12) suSeviyeCm = 12;

  suSeviyesiYuzde = percentTable[suSeviyeCm];
}

void updateLCD() {
  // LCD titremesini önlemek için sadece gerektiğinde temizle veya üzerine yaz
  lcd.setCursor(0, 0);
  lcd.print("Su:%");
  if (suSeviyesiYuzde < 100) lcd.print(" ");
  if (suSeviyesiYuzde < 10)  lcd.print(" ");
  lcd.print(suSeviyesiYuzde);
  
  lcd.print(" ");
  lcd.print(digitalRead(POMPA_PIN) ? "Pmp:ON " : "Pmp:OFF");

  lcd.setCursor(0, 1);
  if (!genelYangin) {
    lcd.print("Durum: GUVENLI  "); 
  } else {
    lcd.print("YANGIN: ");
    if (solDurum) lcd.print("SOL ");
    else if (ortaDurum) lcd.print("ORTA");
    else if (sagDurum) lcd.print("SAG ");
  }
}

void extinguishFire(int angle) {
  // Manuel modda ise yangın söndürme rutinini asla çalıştırma
  if (manuelMod) return; 

  myServo.write(angle);
  delay(200); 
  digitalWrite(POMPA_PIN, HIGH); 
  delay(100);
  
  // Servo tarama hareketi
  for (int pos = angle - 15; pos <= angle + 15; pos += 5) {
    if (manuelMod) break; // İşlem sırasında manuele geçilirse dur
    myServo.write(pos);
    delay(30);
  }
  for (int pos = angle + 15; pos >= angle - 15; pos -= 5) {
    if (manuelMod) break;
    myServo.write(pos);
    delay(30);
  }
}