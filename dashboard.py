import paho.mqtt.client as mqtt
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import pandas as pd
import numpy as np
import json
import random
import csv  # --- YENİ EKLENEN: CSV Kütüphanesi
import os   # --- YENİ EKLENEN: Dosya kontrolü için
from datetime import datetime, timedelta

# ================= AYARLAR =================
MQTT_BROKER = "test.mosquitto.org"
MQTT_PORT = 1883
MQTT_TOPIC = "okulproje/istasyon/veri"
DOSYA_ADI = "akilli_istasyon_verileri.csv" # Excel dosyasının adı

# ================= GLOBAL DEĞİŞKENLER =================
canli_veri = {"su": 0, "yangin": 0, "pompa": 0}
canli_zamanlar = []
canli_su_seviyeleri = []

# ================= 1. BÖLÜM: BÜYÜK VERİ SİMÜLASYONU VE KAYIT =================
def gecmis_veri_ucret():
    print("Büyük Veri (Big Data) simülasyonu hazırlanıyor...")
    
    bitis_zamani = datetime.now()
    baslangic_zamani = bitis_zamani - timedelta(days=7) 
    
    kayitlar = []
    mevcut_zaman = baslangic_zamani
    simule_su = 80 
    
    while mevcut_zaman <= bitis_zamani:
        saat = mevcut_zaman.hour
        if 8 <= saat <= 22: 
            degisim = random.uniform(-0.5, 0.1) 
        else: 
            degisim = random.uniform(-0.1, 0.05)
            
        simule_su += degisim
        if simule_su < 10: simule_su = 90 
        if simule_su > 100: simule_su = 100
        
        yangin = 0
        pompa = 0
        if random.random() < 0.002: 
            yangin = 1
            pompa = 1
            
        kayitlar.append({
            "Zaman": mevcut_zaman.strftime("%Y-%m-%d %H:%M:%S"), # Excel formatı
            "SuSeviyesi": round(simule_su, 2),
            "Yangin": yangin,
            "Pompa": pompa
        })
        
        mevcut_zaman += timedelta(minutes=1)
        
    df = pd.DataFrame(kayitlar)
    
    # --- YENİ EKLENEN: Verileri CSV dosyasına kaydetme ---
    # Eğer dosya yoksa sıfırdan oluştur, varsa üzerine yazma (w modu)
    df.to_csv(DOSYA_ADI, index=False, sep=",") 
    print(f"Toplam {len(df)} satirlik veri '{DOSYA_ADI}' dosyasina kaydedildi.")
    
    # Grafik çizimi için zaman formatını geri çeviriyoruz
    df['Zaman'] = pd.to_datetime(df['Zaman'])
    return df

df_gecmis = gecmis_veri_ucret()

# ================= 2. BÖLÜM: MQTT BAĞLANTISI VE CANLI KAYIT =================
def on_connect(client, userdata, flags, rc):
    print(f"MQTT Sunucusuna Bağlandı! Kod: {rc}")
    client.subscribe(MQTT_TOPIC)

def on_message(client, userdata, msg):
    global canli_veri
    try:
        payload = msg.payload.decode()
        # print(f"Gelen Veri: {payload}") # Konsolu kirletmemek için kapattım
        yeni_veri = json.loads(payload)
        
        # Verileri güncelle
        canli_veri["su"] = int(yeni_veri.get("su", 0))
        canli_veri["yangin"] = int(yeni_veri.get("yangin", 0))
        canli_veri["pompa"] = int(yeni_veri.get("pompa", 0))
        
        # Grafik listelerini güncelle
        canli_zamanlar.append(datetime.now())
        canli_su_seviyeleri.append(canli_veri["su"])
        if len(canli_zamanlar) > 50:
            canli_zamanlar.pop(0)
            canli_su_seviyeleri.pop(0)

        # --- YENİ EKLENEN: CANLI VERİYİ DOSYAYA EKLEME (APPEND) ---
        # Dosyayı 'a' (append/ekle) modunda açıyoruz
        with open(DOSYA_ADI, mode='a', newline='') as dosya:
            yazici = csv.writer(dosya, delimiter=',')
            simdiki_zaman = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            
            # Excel'e yeni satır ekle: Zaman, Su, Yangin, Pompa
            yazici.writerow([simdiki_zaman, canli_veri["su"], canli_veri["yangin"], canli_veri["pompa"]])
            # Terminale de bilgi verelim
            print(f"Excel'e Kaydedildi -> Saat: {simdiki_zaman} | Su: {canli_veri['su']}")

    except Exception as e:
        print("Veri hatası:", e)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect(MQTT_BROKER, MQTT_PORT, 60)
client.loop_start()

# ================= 3. BÖLÜM: GÖRSELLEŞTİRME (DASHBOARD) =================
plt.style.use('dark_background')
fig = plt.figure(figsize=(12, 8))
fig.canvas.manager.set_window_title('Akıllı İtfaiye - Büyük Veri ve Raporlama')

gs = fig.add_gridspec(2, 2)

# Grafik 1: Geçmiş Veri
ax1 = fig.add_subplot(gs[0, :])
ax1.set_title(f"Veritabanı Analizi ({len(df_gecmis)} Kayıt)", color='cyan')
ax1.plot(df_gecmis['Zaman'], df_gecmis['SuSeviyesi'], color='green', alpha=0.7)
ax1.set_ylabel('Su Seviyesi (%)')
ax1.grid(True, alpha=0.3)

yangin_anlari = df_gecmis[df_gecmis['Yangin'] == 1]
ax1.scatter(yangin_anlari['Zaman'], yangin_anlari['SuSeviyesi'], color='red', label='Yangın Olayı', zorder=5)
ax1.legend()

# Grafik 2: Canlı Su
ax2 = fig.add_subplot(gs[1, 0])
def guncelle_grafik(frame):
    ax2.clear()
    ax2.set_title("CANLI: Su Seviyesi", color='yellow')
    if len(canli_zamanlar) > 1:
        ax2.plot(canli_zamanlar, canli_su_seviyeleri, color='cyan', linewidth=2)
        ax2.fill_between(canli_zamanlar, canli_su_seviyeleri, color='cyan', alpha=0.2)
    ax2.set_ylim(0, 100)
    ax2.grid(True, alpha=0.3)
    
    su_anlik = canli_veri["su"]
    ax2.text(0.5, 0.5, f"%{su_anlik}", transform=ax2.transAxes, ha='center', va='center', fontsize=30, color='white', alpha=0.5)

# Grafik 3: Durum
ax3 = fig.add_subplot(gs[1, 1])
ax3.axis('off')

def guncelle_durum(frame):
    ax3.clear()
    ax3.axis('off')
    durum_yangin = canli_veri["yangin"]
    
    if durum_yangin == 1:
        renk = 'red'
        metin = "YANGIN VAR!\nKAYIT ALINIYOR..."
    else:
        renk = 'lime'
        metin = "SİSTEM GÜVENLİ\nVERİ TABANI AKTİF"
        
    ax3.text(0.5, 0.5, metin, ha='center', va='center', fontsize=18, color='white', bbox=dict(facecolor=renk, alpha=0.5, boxstyle='round,pad=1'))

ani1 = animation.FuncAnimation(fig, guncelle_grafik, interval=1000)
ani2 = animation.FuncAnimation(fig, guncelle_durum, interval=1000)

plt.tight_layout()
plt.show()