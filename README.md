# AI-Gas-Meter-Tracking-System-ESP32-CAM
Gas or water meter tracking application for ESP32-CAM. 

Bu proje, ESP32-CAM kullanarak analog doğalgaz (veya su/gaz) sayaçlarını yapay zeka ile otonom olarak okuyan, web arayüzünden takip edilmesini sağlayan ve aylık kota aşıldığında **fotoğraflı e-posta uyarısı** gönderen bir IoT sistemidir.

##  Özellikler
* **Çift Yapay Zeka Motoru:** İnternet yokken yerel TFLite modeliyle (*henüz deneme aşamasında*), internet varken Google Gemini Flash Lite API'si ile sayaç endeksi.
* **Otonom Çalışma:** Sizin belirlediğiniz saatte arka planda uyanır, fotoğrafı çeker, okur ve sonuçları SD karta kaydeder.
* **Endüstriyel Kalibrasyon Paneli:** TF-Lite için tarayıcı üzerinden sürükle-bırak yöntemiyle rakam kutularını (ROI) ayarlama ve ince hizalama (Rotation/Açı Döndürme) ızgarası.
* **Akıllı Kota & Mail Sistemi:** 12 aylık kotalar belirlenebilir. Kota %90'a (veya ayarlanan değere) ulaştığında Google Webhook üzerinden anında fotoğraflı mail gönderir.
* **Gelişmiş Geçmiş Tablosu:** Her okuma geçmişe kaydedilir. Fatura dönemi değiştiğinde tabloda otomatik ayrım çizgisi oluşur ve tıklanan okumanın geçmiş fotoğrafı Modal penceresinde izlenebilir.
* **Elektrik Kesintisine Karşı Korumalı:** Tüm ayarlar ve geçmiş veriler SD kartta güvenle saklanır.

<img width="239" height="379" alt="res1" src="https://github.com/user-attachments/assets/3925cb88-b3d3-4692-a110-8c47b4c5e465" />
 <img width="243" height="384" alt="res2" src="https://github.com/user-attachments/assets/d11d3e12-5775-46f3-8f78-21db52a25667" /><img width="237" height="409" alt="res5" src="https://github.com/user-attachments/assets/23efc36a-2695-4c05-8258-932b1bd27ed3" />
<img width="244" height="284" alt="res4" src="https://github.com/user-attachments/assets/060043e0-1bd8-45ee-b5fa-6d4765b113bc" />
<img width="241" height="410" alt="res3" src="https://github.com/user-attachments/assets/4b9b70f6-9855-404c-94cd-3ccad2565fdd" />




## 🛠️ Kurulum Adımları
1. Kodları indirin ve Arduino IDE ile ESP32-CAM kartınıza yükleyin. Yüklemeden önce `Pro_Dogalgaz_Takip.ino` içindeki Wi-Fi SSID ve Şifrenizi değiştirmeyi unutmayın.
2. `web` klasörünün içindeki dosyaları (`index.html`, `app.js`, `style.css`) bir SD kartın ana dizininde `web` adında bir klasör oluşturarak içine atın.
3. (İsteğe Bağlı) `Google_Apps_Script.js` kodunu Google Apps Script üzerinden yayınlayarak oluşan Webhook URL'sini arayüzdeki E-Posta ayarlarına yapıştırın.
4. Cihaza güç verin ve belirlediğiniz IP adresi üzerinden arayüze bağlanın!

## ⚠️ Donanım Tavsiyesi
ESP32-CAM üzerindeki dahili anten zayıf kalabileceğinden, doğalgaz kutusu gibi engelleri aşmak için mutlaka **IPEX (U.FL) Harici Anten** modifikasyonunu yapmanız önerilir. Cihazı stabil beslemek için 5V 2A adaptör kullanın.
