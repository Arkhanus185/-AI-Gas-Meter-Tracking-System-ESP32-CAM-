#include "esp_camera.h"
#include "FS.h"
#include "SD_MMC.h"
#include <WiFi.h>
#include <WebServer.h>
#include "time.h"
#include <ArduinoJson.h>

// --- AĞ AYARLARI ---
const char* ssid = "2.4GHz wifi adınız";
const char* password = "wifi sifresi";

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
int config_autoUpdateFatura = 0; // YENİ: Ay Sonu Otomatik Güncelleme Checkbox (0:Kapalı, 1:Açık)

String last_auto_trigger_date = "";
int last_checked_month = -1;
long photo_update_time = 0; 
bool isConfigLoaded = false; // Güvenlik Kilidi

#ifdef __cplusplus
extern "C" { uint8_t temprature_sens_read(); }
#endif

#include "Hafiza_Ayarlar.h"
#include "Kamera_Kontrol.h"
#include "YapayZeka_Motoru.h" 
#include "Web_Sunucu.h"       

void performAutoReading() {
  addLog("Otomatik Zamanlayici Tetiklendi.");
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
  
  String resultJson = (config_aiEngine == "gemini") ? askGemini(String(fileName)) : askTFLite(String(fileName));
  photo_update_time = millis(); 
  addLog("Otomatik Okuma Tamamlandi.");
}

void setup() {
  Serial.begin(115200);
  if(!SD_MMC.begin("/sdcard", true)){ Serial.println("HATA: SD Kart Yok!"); return; }
  if(!SD_MMC.exists("/images")) SD_MMC.mkdir("/images");
  if(!SD_MMC.exists("/config")) SD_MMC.mkdir("/config");

  loadConfig(); 
  initKamera(); 

  WiFi.config(local_IP, gateway, subnet, dns);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  
  configTime(gmtOffset_sec, 0, ntpServer);
  addLog("Sistem baslatildi. IP: " + WiFi.localIP().toString());
  initWebServer(); server.begin();
}

void loop() {
  server.handleClient(); 
  
  static unsigned long lastTimeCheck = 0;
  if (millis() - lastTimeCheck > 10000) { 
    lastTimeCheck = millis();
    struct tm timeinfo;
    if(getLocalTime(&timeinfo)){
      char timeStr[6]; strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
      char dateStr[11]; strftime(dateStr, sizeof(dateStr), "%d.%m.%Y", &timeinfo);
      int currentMonth = timeinfo.tm_mon; // 0-11
      
      // YENİ: AY SONU OTOMATİK FATURA ENDEKSİ GÜNCELLEME KONTROLÜ
      if(last_checked_month != -1 && last_checked_month != currentMonth) {
        if(config_autoUpdateFatura == 1 && config_lastValue != "0" && config_lastValue != "") {
            config_faturaEndeks = config_lastValue;
            config_faturaTarih = String(dateStr);
            saveSistemConfig();
            addLog("Yeni ay basladi! Fatura endeksi otomatik guncellendi: " + config_faturaEndeks);
        }
      }
      last_checked_month = currentMonth;

      // ZAMANLAYICI KONTROLÜ
      if(String(timeStr) == config_autoTime && String(dateStr) != last_auto_trigger_date) {
        last_auto_trigger_date = String(dateStr);
        performAutoReading(); 
      }
    }
  }
}