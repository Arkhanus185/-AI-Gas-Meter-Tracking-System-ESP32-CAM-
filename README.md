# AI-Gas-Meter-Tracking-System-ESP32-CAM
Gas or water meter tracking application for ESP32-CAM. 

Bu proje, ESP32-CAM kullanarak analog doğalgaz (veya su/gaz) sayaçlarını yapay zeka ile otonom olarak okuyan, web arayüzünden takip edilmesini sağlayan ve aylık kota aşıldığında **fotoğraflı e-posta uyarısı** gönderen bir IoT sistemidir.

##  Özellikler
* **Çift Yapay Zeka Motoru:** İnternet yokken yerel TFLite modeliyle (henüz deneme aşamasında), internet varken Google Gemini Flash Lite API'si ile sayaç endeksi.
* **Otonom Çalışma:** Sizin belirlediğiniz saatte arka planda uyanır, fotoğrafı çeker, okur ve sonuçları SD karta kaydeder.
* **Endüstriyel Kalibrasyon Paneli:** TF-Lite için tarayıcı üzerinden sürükle-bırak yöntemiyle rakam kutularını (ROI) ayarlama ve ince hizalama (Rotation/Açı Döndürme) ızgarası.
* **Akıllı Kota & Mail Sistemi:** 12 aylık kotalar belirlenebilir. Kota %90'a (veya ayarlanan değere) ulaştığında Google Webhook üzerinden anında fotoğraflı mail gönderir.
* **Gelişmiş Geçmiş Tablosu:** Her okuma geçmişe kaydedilir. Fatura dönemi değiştiğinde tabloda otomatik ayrım çizgisi oluşur ve tıklanan okumanın geçmiş fotoğrafı Modal penceresinde izlenebilir.
* **Elektrik Kesintisine Karşı Korumalı:** Tüm ayarlar ve geçmiş veriler SD kartta güvenle saklanır.

## 🛠️ Kurulum Adımları
1. Kodları indirin ve Arduino IDE ile ESP32-CAM kartınıza yükleyin. Yüklemeden önce `Pro_Dogalgaz_Takip.ino` içindeki Wi-Fi SSID ve Şifrenizi değiştirmeyi unutmayın.
2. `web` klasörünün içindeki dosyaları (`index.html`, `app.js`, `style.css`) bir SD kartın ana dizininde `web` adında bir klasör oluşturarak içine atın.
3. (İsteğe Bağlı) `Google_Apps_Script.js` kodunu Google Apps Script üzerinden yayınlayarak oluşan Webhook URL'sini arayüzdeki E-Posta ayarlarına yapıştırın.
4. Cihaza güç verin ve belirlediğiniz IP adresi üzerinden arayüze bağlanın!

## ⚠️ Donanım Tavsiyesi
ESP32-CAM üzerindeki dahili anten zayıf kalabileceğinden, doğalgaz kutusu gibi engelleri aşmak için mutlaka **IPEX (U.FL) Harici Anten** modifikasyonunu yapmanız önerilir. Cihazı stabil beslemek için 5V 2A adaptör kullanın.
