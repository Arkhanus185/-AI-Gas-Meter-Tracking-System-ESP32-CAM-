// --- SD KART, LOG VE GEÇMİŞ YÖNETİM MODÜLÜ ---

void saveSistemConfig(); // YENİ: Derleyiciyi uyaran ön bildirim (Forward Declaration) satırı

void addLog(String message) {
  File checkFile = SD_MMC.open("/log.txt", FILE_READ);
  if(checkFile && checkFile.size() > 50000) { checkFile.close(); SD_MMC.remove("/log.txt"); } 
  else if(checkFile) { checkFile.close(); }
  
  struct tm timeinfo; String timeStr = "Zaman Yok";
  if(getLocalTime(&timeinfo)){
    char tBuff[50]; strftime(tBuff, sizeof(tBuff), "%Y-%m-%d %H:%M:%S", &timeinfo);
    timeStr = String(tBuff);
  }
  String logEntry = "[" + timeStr + "] " + message + "\n";
  Serial.print(logEntry);
  File logFile = SD_MMC.open("/log.txt", FILE_APPEND);
  if(logFile){ logFile.print(logEntry); logFile.close(); }
}

void loadConfig() {
  if (SD_MMC.exists("/config.json")) {
    File file = SD_MMC.open("/config.json", FILE_READ);
    StaticJsonDocument<2048> doc; 
    if (!deserializeJson(doc, file)) {
      config_aiEngine = doc["aiEngine"] | "gemini";
      config_apiKey = doc["apiKey"] | "";
      config_autoTime = doc["autoTime"] | "14:00";
      config_framesize = 6; 
      config_quality = doc["quality"] | 12; 
      config_useFlash = doc["useFlash"] | 0;
      config_flashPWM = doc["flashPWM"] | 10;
      config_waitTime = doc["waitTime"] | 1000;
      config_rotation = doc["rotation"] | 0.0;
      config_lastValue = doc["lastValue"] | "0";
      config_faturaEndeks = doc["faturaEndeks"] | "";
      config_faturaTarih = doc["faturaTarih"] | "";
      
      config_webhookUrl = doc["webhookUrl"] | "";
      config_lastMailMonth = doc["lastMailMonth"] | -1;
      config_autoUpdateFatura = doc["autoUpdateFatura"] | 0;
      config_mailYuzde = doc["mailYuzde"] | 90;
      config_mailListesi = doc["mailListesi"] | "";
      if(doc.containsKey("kotalar")) {
          for(int i=0; i<12; i++) config_kotalar[i] = doc["kotalar"][i];
      }
      isConfigLoaded = true; 
      addLog("Ayarlar SD karttan basariyla yuklendi.");
    }
    file.close();
  }
}

void modifyHistory(int targetIndex, String newValue, bool isDelete) {
  if(!SD_MMC.exists("/history.csv")) return;
  
  File file = SD_MMC.open("/history.csv", FILE_READ);
  String lines[100]; int count = 0;
  while(file.available() && count < 100) { lines[count++] = file.readStringUntil('\n'); }
  file.close();

  SD_MMC.remove("/history.csv");
  File newFile = SD_MMC.open("/history.csv", FILE_WRITE);
  
  String latestEndeks = "0"; // Yapay zeka hafızası için son geçerli değer takibi

  for(int i=0; i<count; i++) {
    if(lines[i].length() < 5) continue;
    
    if(i == targetIndex) {
      if(!isDelete) {
        int firstComma = lines[i].indexOf(',');
        int secondComma = lines[i].indexOf(',', firstComma + 1);
        int thirdComma = lines[i].indexOf(',', secondComma + 1);
        int fourthComma = lines[i].indexOf(',', thirdComma + 1);
        
        String date = lines[i].substring(0, firstComma);
        String faturaEnd = (fourthComma != -1) ? lines[i].substring(thirdComma + 1, fourthComma) : config_faturaEndeks;
        String fileLink = (fourthComma != -1) ? lines[i].substring(fourthComma + 1) : lines[i].substring(thirdComma + 1);
        
        String newTuketim = "-";
        if(faturaEnd != "") {
          newTuketim = String(newValue.toInt() - faturaEnd.toInt());
        }
        newFile.print(date + "," + newValue + "," + newTuketim + "," + faturaEnd + "," + fileLink + "\n");
        latestEndeks = newValue; 
        addLog("Tablo Duzenlendi: " + newValue);
      }
    } else {
      newFile.print(lines[i] + "\n"); 
      
      // Satırdaki endeks değerini ayıkla ve son geçerli değer olarak kaydet
      int firstComma = lines[i].indexOf(',');
      int secondComma = lines[i].indexOf(',', firstComma + 1);
      if (firstComma != -1 && secondComma != -1) {
          latestEndeks = lines[i].substring(firstComma + 1, secondComma);
      }
    }
  }
  newFile.close();

  // Tablo düzenlendikten sonra yapay zekanın "Son Değer" hafızasını senkronize et
  if (latestEndeks != "0" && latestEndeks != "") {
      config_lastValue = latestEndeks;
      saveSistemConfig();
      addLog("Yapay Zeka Referans Hafizasi Guncellendi: " + config_lastValue);
  }
}