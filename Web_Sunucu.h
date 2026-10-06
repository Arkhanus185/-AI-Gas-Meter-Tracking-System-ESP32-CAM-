// --- WEB SUNUCUSU VE ARAYÜZ YÖNETİM MODÜLÜ ---

void initWebServer() {
  
  server.on("/", HTTP_GET, []() { 
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    File f = SD_MMC.open("/web/index.html", FILE_READ); server.streamFile(f, "text/html"); f.close(); 
  });
  server.on("/style.css", HTTP_GET, []() { 
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    File f = SD_MMC.open("/web/style.css", FILE_READ); server.streamFile(f, "text/css"); f.close(); 
  });
  server.on("/app.js", HTTP_GET, []() { 
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    File f = SD_MMC.open("/web/app.js", FILE_READ); server.streamFile(f, "application/javascript"); f.close(); 
  });
  
  server.on("/log", HTTP_GET, []() { File f = SD_MMC.open("/log.txt", FILE_READ); server.streamFile(f, "text/plain"); f.close(); });
  server.on("/history", HTTP_GET, []() {
    if(SD_MMC.exists("/history.csv")){ File f = SD_MMC.open("/history.csv", FILE_READ); server.streamFile(f, "text/csv"); f.close(); } 
    else { server.send(200, "text/csv", ""); }
  });

  server.on("/status", HTTP_GET, []() {
    float cpu_temp = (temprature_sens_read() - 32) / 1.8; 
    long rssi = WiFi.RSSI();
    server.send(200, "application/json", "{\"cpu_temp\":\"" + String(cpu_temp, 1) + "\",\"rssi\":\"" + String(rssi) + "\",\"photo_time\":\"" + String(photo_update_time) + "\"}");
  });

  server.on("/get_config", HTTP_GET, []() {
    StaticJsonDocument<2048> doc;
    doc["aiEngine"] = config_aiEngine; doc["apiKey"] = config_apiKey; doc["autoTime"] = config_autoTime; 
    doc["framesize"] = config_framesize; doc["quality"] = config_quality; 
    doc["useFlash"] = config_useFlash; doc["flashPWM"] = config_flashPWM; doc["waitTime"] = config_waitTime; 
    doc["rotation"] = config_rotation;
    doc["faturaEndeks"] = config_faturaEndeks; doc["faturaTarih"] = config_faturaTarih;
    doc["autoUpdateFatura"] = config_autoUpdateFatura;
    
    doc["mailYuzde"] = config_mailYuzde;
    doc["mailListesi"] = config_mailListesi;
    doc["webhookUrl"] = config_webhookUrl;
    JsonArray kotalarArr = doc.createNestedArray("kotalar");
    for(int i=0; i<12; i++) kotalarArr.add(config_kotalar[i]);

    String res; serializeJson(doc, res); server.send(200, "application/json", res);
  });

  server.on("/set_config", HTTP_POST, []() {
    StaticJsonDocument<2048> doc; deserializeJson(doc, server.arg("plain"));
    config_aiEngine = doc["aiEngine"].as<String>(); config_apiKey = doc["apiKey"].as<String>(); config_autoTime = doc["autoTime"].as<String>(); 
    config_framesize = doc["framesize"]; config_quality = doc["quality"]; 
    config_useFlash = doc["useFlash"]; config_flashPWM = doc["flashPWM"]; config_waitTime = doc["waitTime"]; 
    config_rotation = doc["rotation"] | 0.0;
    config_faturaEndeks = doc["faturaEndeks"].as<String>(); config_faturaTarih = doc["faturaTarih"].as<String>();
    config_autoUpdateFatura = doc["autoUpdateFatura"] | 0;
    
    config_mailYuzde = doc["mailYuzde"];
    config_mailListesi = doc["mailListesi"].as<String>();
    config_webhookUrl = doc["webhookUrl"].as<String>();
    if(doc.containsKey("kotalar")) { for(int i=0; i<12; i++) config_kotalar[i] = doc["kotalar"][i]; }

    File f = SD_MMC.open("/config.json", FILE_WRITE); doc["lastValue"] = config_lastValue; serializeJson(doc, f); f.close(); 
    applyCameraSettings((framesize_t)config_framesize); 
    addLog("Ayarlar arayuzden kaydedildi."); server.send(200, "text/plain", "OK");
  });

  server.on("/set_roi", HTTP_POST, []() {
    File f = SD_MMC.open("/config/roi.json", FILE_WRITE); f.print(server.arg("plain")); f.close();
    addLog("Kalibrasyon Koordinatlari SD Karta Kaydedildi."); server.send(200, "text/plain", "OK");
  });

  server.on("/preview", HTTP_GET, []() {
    camera_fb_t * fb = esp_camera_fb_get(); if (!fb) { server.send(500); return; }
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.setContentLength(fb->len); server.send(200, "image/jpeg", ""); server.client().write(fb->buf, fb->len);
    esp_camera_fb_return(fb);
  });

  server.on("/capture_reference", HTTP_GET, []() {
    if(config_useFlash) { ledcWrite(pwmChannel, config_flashPWM); delay(config_waitTime); }
    camera_fb_t * fb = NULL;
    for(int i=0; i<3; i++) {
      fb = esp_camera_fb_get();
      if(i < 2) { esp_camera_fb_return(fb); delay(100); } 
    }
    if(config_useFlash) ledcWrite(pwmChannel, 0);
    if (!fb) { server.send(500); return; }
    
    SD_MMC.remove("/config/reference.jpg");
    File refFile = SD_MMC.open("/config/reference.jpg", FILE_WRITE);
    refFile.write(fb->buf, fb->len); refFile.close();
    server.send(200, "application/json", "{\"status\":\"OK\"}");
    esp_camera_fb_return(fb);
  });

  server.on("/capture", HTTP_GET, []() {
    if(config_useFlash) { ledcWrite(pwmChannel, config_flashPWM); delay(config_waitTime); }
    camera_fb_t * fb = NULL;
    for(int i=0; i<3; i++) {
      fb = esp_camera_fb_get();
      if(i < 2) { esp_camera_fb_return(fb); delay(100); } 
    }
    if(config_useFlash) ledcWrite(pwmChannel, 0); 
    if (!fb) { server.send(500); return; }

    struct tm timeinfo; char fileName[50]; char timeStr[50];
    if(getLocalTime(&timeinfo)){ strftime(fileName, sizeof(fileName), "/images/%Y%m%d_%H%M%S.jpg", &timeinfo); strftime(timeStr, sizeof(timeStr), "%d.%m.%Y %H:%M:%S", &timeinfo); } 
    else { sprintf(fileName, "/images/latest.jpg"); sprintf(timeStr, "Zaman Yok"); }

    File file = SD_MMC.open(fileName, FILE_WRITE); file.write(fb->buf, fb->len); file.close();
    SD_MMC.remove("/latest.jpg"); File latest = SD_MMC.open("/latest.jpg", FILE_WRITE); latest.write(fb->buf, fb->len); latest.close();
    
    photo_update_time = millis(); 

    StaticJsonDocument<256> doc; doc["url"] = String(fileName); doc["filename"] = String(fileName); doc["zaman"] = String(timeStr);
    String res; serializeJson(doc, res); server.send(200, "application/json", res);
    esp_camera_fb_return(fb); 
  });

  server.on("/update_history", HTTP_GET, []() { modifyHistory(server.arg("index").toInt(), server.arg("val"), false); server.send(200, "text/plain", "OK"); });
  server.on("/delete_history", HTTP_GET, []() { modifyHistory(server.arg("index").toInt(), "", true); server.send(200, "text/plain", "OK"); });

  server.on("/ask_ai", HTTP_GET, []() {
    String fileName = server.arg("file");
    if(!SD_MMC.exists(fileName)) { server.send(200, "application/json", "{\"hata\":\"Dosya Yok\"}"); return; }
    String resultJson = (config_aiEngine == "gemini") ? askGemini(fileName) : askTFLite(fileName);
    server.send(200, "application/json", resultJson);
  });

  server.onNotFound([]() {
    String p = server.uri();
    if(SD_MMC.exists(p)){
      String mime = "image/jpeg";
      if(p.endsWith(".css")) mime = "text/css"; else if(p.endsWith(".js")) mime = "application/javascript"; else if(p.endsWith(".html")) mime = "text/html";
      File f = SD_MMC.open(p, FILE_READ); server.streamFile(f, mime); f.close();
    } else { server.send(404, "text/plain", "Bulunamadi"); }
  });

  server.begin();
}