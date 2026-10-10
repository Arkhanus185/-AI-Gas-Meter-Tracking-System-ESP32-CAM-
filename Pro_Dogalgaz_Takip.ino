#include "esp_camera.h"
#include "FS.h"
#include "SD_MMC.h"
#include <WiFi.h>
#include <WebServer.h>
#include "time.h"
#include <ArduinoJson.h>
#include "esp_task_wdt.h" // YENİ: Donanım Bekçisi (Watchdog) Kütüphanesi

// --- AĞ AYARLARI ---
const char* ssid = "Wifi_adi";
const char* password = "wifi_sifresi";

IPAddress local_IP(192, 168, 1, 55);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 1, 1);

WebServer server(80);
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 10800; // GMT+3

// --- GLOBAL AYAR DEĞİŞKENLERİ ---
String config_aiEngine = "gemini"; 
String config_apiKey = "";
String config_autoTime = "14:00";
int config_framesize = 6; 
int config_quality = 12;
int config_useFlash = 0;    
int config_flashPWM = 10;   
int config_waitTime = 1000;
float config_rotation = 0.0; 
String config_lastValue = "0"; 
String config_faturaEndeks = "";
String config_faturaTarih = "";

int config_kotalar[12] = {0,0,0,0,0,0,0,0,0,0,0,0}; 
int config_mailYuzde = 90; 
String config_mailListesi = ""; 
String config_webhookUrl = ""; 
int config_lastMailMonth = -1; 
int config_autoUpdateFatura = 0; 

String last_auto_trigger_date = "";
int last_checked_month = -1;
long photo_update_time = 0; 
bool isConfigLoaded = false; 

// YENİ: Wi-Fi Kopma Takibi İçin Sayaç
unsigned long previousMillis = 0;
const long interval = 30000; // 30 saniyede bir Wi-Fi kontrolü

#ifdef __cplusplus
extern "C" { uint8_t temprature_sens_read(); }
#endif

#include "Hafiza_Ayarlar.h"
#include "Kamera_Kontrol.h"
#include "YapayZeka_Motoru.h" 
#include "Web_Sunucu.h"       

void performAutoReading() {
  addLog("Otomatik Zamanlayici Tetiklendi.");
  
  // İşlem uzun süreceği için Bekçi Köpeğini sıfırla
  esp_task_wdt_reset(); 
  
  applyCameraSettings((framesize_t)config_framesize); delay(200); 
  if(config_useFlash) { ledcWrite(pwmChannel, config_flashPWM); delay(config_waitTime); }
  
  camera_fb_t * fb = NULL;
  for(int i=0; i<3; i++) {
    fb = esp_camera_fb_get();
    if(i < 2) { esp_camera_fb_return(fb); delay(100); } 
  }
  if(config_useFlash) ledcWrite(pwmChannel, 0); 
  if (!fb) { addLog("HATA: Otomatik cekimde kamera arizasi!"); return; }

  struct tm timeinfo; char fileName[50];
  if(getLocalTime(&timeinfo)){ strftime(fileName, sizeof(fileName), "/images/%Y%m%d_%H%M%S.jpg", &timeinfo); } 
  else { sprintf(fileName, "/images/latest.jpg"); }

  File file = SD_MMC.open(fileName, FILE_WRITE); file.write(fb->buf, fb->len); file.close();
  SD_MMC.remove("/latest.jpg"); File latest = SD_MMC.open("/latest.jpg", FILE_WRITE); latest.write(fb->buf, fb->len); latest.close();
  esp_camera_fb_return(fb); 
  
  esp_task_wdt_reset(); // Gemini/TFLite öncesi Bekçiyi tekrar sıfırla
  String resultJson = (config_aiEngine == "gemini") ? askGemini(String(fileName)) : askTFLite(String(fileName));
  
  photo_update_time = millis(); 
  addLog("Otomatik Okuma Tamamlandi.");
}

void setup() {
  Serial.begin(115200);
  
  // YENİ: Watchdog (Bekçi Köpeği) Kurulumu
  // 45 Saniye boyunca sistemden haber alınamazsa ESP32'yi zorla Resetler!
  esp_task_wdt_init(45, true); 
  esp_task_wdt_add(NULL);      

  if(!SD_MMC.begin("/sdcard", true)){ Serial.println("HATA: SD Kart Yok!"); return; }
  if(!SD_MMC.exists("/images")) SD_MMC.mkdir("/images");
  if(!SD_MMC.exists("/config")) SD_MMC.mkdir("/config");

  loadConfig(); 
  initKamera(); 

  WiFi.config(local_IP, gateway, subnet, dns);
  
  // YENİ: Wi-Fi'nin Kendi Kendine Bağlanma Modunu Aktif Et
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  
  WiFi.begin(ssid, password);
  int connectCount = 0;
  while (WiFi.status() != WL_CONNECTED && connectCount < 20) { 
    delay(500); 
    Serial.print("."); 
    connectCount++;
  }
  
  configTime(gmtOffset_sec, 0, ntpServer);
  addLog("Sistem baslatildi. IP: " + WiFi.localIP().toString());
  initWebServer(); server.begin();
}

void loop() {
  server.handleClient(); 
  
  // YENİ: Her döngüde Bekçi Köpeğine "Ben Yaşıyorum" sinyali gönder!
  esp_task_wdt_reset(); 
  
  unsigned long currentMillis = millis();
  
  // YENİ: Wi-Fi Kopma Kontrolü (Her 30 Saniyede Bir)
  if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      if (WiFi.status() != WL_CONNECTED) {
          addLog("UYARI: Wi-Fi baglantisi koptu! Yeniden baglaniliyor...");
          WiFi.disconnect();
          WiFi.begin(ssid, password);
      }
  }
  
  static unsigned long lastTimeCheck = 0;
  if (currentMillis - lastTimeCheck > 10000) { 
    lastTimeCheck = currentMillis;
    struct tm timeinfo;
    if(getLocalTime(&timeinfo)){
      char timeStr[6]; strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
      char dateStr[11]; strftime(dateStr, sizeof(dateStr), "%d.%m.%Y", &timeinfo);
      int currentMonth = timeinfo.tm_mon; 
      
      if(last_checked_month != -1 && last_checked_month != currentMonth) {
        if(config_autoUpdateFatura == 1 && config_lastValue != "0" && config_lastValue != "") {
            config_faturaEndeks = config_lastValue;
            config_faturaTarih = String(dateStr);
            saveSistemConfig();
            addLog("Yeni ay basladi! Fatura endeksi otomatik guncellendi: " + config_faturaEndeks);
        }
      }
      last_checked_month = currentMonth;

      if(String(timeStr) == config_autoTime && String(dateStr) != last_auto_trigger_date) {
        last_auto_trigger_date = String(dateStr);
        performAutoReading(); 
      }
    }
  }
}
