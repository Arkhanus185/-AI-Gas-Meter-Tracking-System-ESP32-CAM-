// --- YAPAY ZEKA (GEMINI & TFLITE) YÖNETİM MODÜLÜ ---
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <base64.h>
#include <ArduinoJson.h>
#include "img_converters.h"

#include "TensorFlowLite_ESP32.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

const tflite::Model* tflite_model = nullptr;
tflite::MicroInterpreter* tflite_interpreter = nullptr;
TfLiteTensor* tflite_input = nullptr;
TfLiteTensor* tflite_output = nullptr;
uint8_t* tensor_arena = nullptr;
const int kTensorArenaSize = 1024 * 1024; 
uint8_t* model_buffer = nullptr;
bool isTFLiteInitialized = false;

// KOTA KONTROLÜ VE GOOGLE APPS SCRIPT WEBHOOK (SAYAÇ RESMİ İLE BİRLİKTE)
void checkAndSendWebhook(String endeks, String tuketim, String fileName) {
    if(tuketim == "" || tuketim == "-" || config_webhookUrl == "" || config_mailListesi == "") return;
    
    struct tm timeinfo;
    if(getLocalTime(&timeinfo)){
        int currentMonth = timeinfo.tm_mon; // 0-11
        int kota = config_kotalar[currentMonth];
        
        if(kota > 0 && config_mailYuzde > 0) {
            int harcanan = tuketim.toInt();
            int sinir = (kota * config_mailYuzde) / 100;
            
            if(harcanan >= sinir && config_lastMailMonth != currentMonth) {
                addLog("DIKKAT! Kota siniri asildi. Resimli Mail Hazirlaniyor...");
                
                String imgBase64 = "";
                File imgFile = SD_MMC.open(fileName, FILE_READ);
                if(imgFile) {
                    size_t fSize = imgFile.size();
                    uint8_t *bBuf = (uint8_t*)ps_malloc(fSize);
                    if(bBuf) {
                        imgFile.read(bBuf, fSize);
                        imgBase64 = base64::encode(bBuf, fSize);
                        free(bBuf);
                    }
                    imgFile.close();
                }

                HTTPClient http;
                http.begin(config_webhookUrl);
                http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
                http.addHeader("Content-Type", "application/json");
                
                size_t pSize = imgBase64.length() + 500;
                char *payload = (char*)ps_malloc(pSize);
                if(payload) {
                    snprintf(payload, pSize, "{\"mailler\":\"%s\", \"endeks\":\"%s\", \"tuketim\":\"%s\", \"kota\":\"%d\", \"sinir\":\"%d\", \"yuzde\":\"%d\", \"imgBase64\":\"%s\"}", 
                             config_mailListesi.c_str(), endeks.c_str(), tuketim.c_str(), kota, sinir, config_mailYuzde, imgBase64.c_str());
                    
                    int httpCode = http.POST((uint8_t*)payload, strlen(payload));
                    free(payload);
                    addLog("Webhook POST Kodu: " + String(httpCode));
                }
                http.end();
                config_lastMailMonth = currentMonth; 
            }
        }
    }
}

int get_jpeg_width_height(const uint8_t *data, size_t size, uint16_t *w, uint16_t *h) {
    size_t i = 0; if (data[i] != 0xFF || data[i+1] != 0xD8) return -1; i += 2;
    while (i < size - 8) { if (data[i] != 0xFF) { i++; continue; } uint8_t marker = data[i+1]; if (marker == 0xFF) { i++; continue; } if (marker == 0xC0 || marker == 0xC2) { *h = (data[i+5] << 8) | data[i+6]; *w = (data[i+7] << 8) | data[i+8]; return 0; } int len = (data[i+2] << 8) | data[i+3]; i += 2 + len; } return -1;
}

String getBmpBase64(uint8_t* rgb, int w, int h) {
    int rowSize = (w * 3 + 3) & ~3; int imgSize = rowSize * h; int fileSize = 54 + imgSize; uint8_t* bmp = (uint8_t*)ps_calloc(fileSize, 1); if(!bmp) return "";
    bmp[0] = 'B'; bmp[1] = 'M'; bmp[2] = fileSize & 0xFF; bmp[3] = (fileSize >> 8) & 0xFF; bmp[4] = (fileSize >> 16) & 0xFF; bmp[5] = (fileSize >> 24) & 0xFF; bmp[10] = 54; bmp[14] = 40; bmp[18] = w & 0xFF; bmp[19] = (w >> 8) & 0xFF; bmp[20] = (w >> 16) & 0xFF; bmp[21] = (w >> 24) & 0xFF; bmp[22] = h & 0xFF; bmp[23] = (h >> 8) & 0xFF; bmp[24] = (h >> 16) & 0xFF; bmp[25] = (h >> 24) & 0xFF; bmp[26] = 1; bmp[28] = 24;
    for(int y = 0; y < h; y++) { for(int x = 0; x < w; x++) { int src = ((h - 1 - y) * w + x) * 3; int dst = 54 + y * rowSize + x * 3; bmp[dst+0] = rgb[src+2]; bmp[dst+1] = rgb[src+1]; bmp[dst+2] = rgb[src+0]; } } String b64 = base64::encode(bmp, fileSize); free(bmp); return "data:image/bmp;base64," + b64;
}

void saveSistemConfig() {
    if(!isConfigLoaded) return; 
    File f = SD_MMC.open("/config.json", FILE_WRITE); if(!f) return; StaticJsonDocument<2048> saveDoc;
    saveDoc["aiEngine"] = config_aiEngine; saveDoc["apiKey"] = config_apiKey; saveDoc["autoTime"] = config_autoTime;
    saveDoc["framesize"] = config_framesize; saveDoc["quality"] = config_quality; saveDoc["useFlash"] = config_useFlash;
    saveDoc["flashPWM"] = config_flashPWM; saveDoc["waitTime"] = config_waitTime; saveDoc["rotation"] = config_rotation; 
    saveDoc["faturaEndeks"] = config_faturaEndeks; saveDoc["faturaTarih"] = config_faturaTarih; saveDoc["lastValue"] = config_lastValue;
    saveDoc["webhookUrl"] = config_webhookUrl; saveDoc["lastMailMonth"] = config_lastMailMonth;
    saveDoc["autoUpdateFatura"] = config_autoUpdateFatura; saveDoc["mailYuzde"] = config_mailYuzde; saveDoc["mailListesi"] = config_mailListesi;
    JsonArray k = saveDoc.createNestedArray("kotalar"); for(int i=0; i<12; i++) k.add(config_kotalar[i]);
    serializeJson(saveDoc, f); f.close();
}

bool iTFLite() {
    if (isTFLiteInitialized) return true; if (!SD_MMC.exists("/config/dig-class100-0182-s2_q.tflite")) return false;
    File modelFile = SD_MMC.open("/config/dig-class100-0182-s2_q.tflite", FILE_READ); size_t modelSize = modelFile.size(); model_buffer = (uint8_t*)ps_malloc(modelSize);
    modelFile.read(model_buffer, modelSize); modelFile.close(); tflite_model = tflite::GetModel(model_buffer); static tflite::MicroErrorReporter micro_error_reporter; static tflite::AllOpsResolver resolver;
    tensor_arena = (uint8_t*)ps_malloc(kTensorArenaSize); tflite_interpreter = new tflite::MicroInterpreter(tflite_model, resolver, tensor_arena, kTensorArenaSize, &micro_error_reporter);
    tflite_interpreter->AllocateTensors(); tflite_input = tflite_interpreter->input(0); tflite_output = tflite_interpreter->output(0); isTFLiteInitialized = true; return true;
}

String askTFLite(String fileName) {
    if(!SD_MMC.exists("/config/roi.json")) return "{\"hata\":\"Kutu cizmelisiniz!\"}"; if (!iTFLite()) return "{\"hata\":\"Model baslatilamadi.\"}";
    File roiFile = SD_MMC.open("/config/roi.json", FILE_READ); String roiData = roiFile.readString(); roiFile.close(); StaticJsonDocument<1024> doc; deserializeJson(doc, roiData); JsonArray rois = doc.as<JsonArray>();
    File imgFile = SD_MMC.open(fileName, FILE_READ); size_t imgSize = imgFile.size(); uint8_t *img_buf = (uint8_t*)ps_malloc(imgSize); imgFile.read(img_buf, imgSize); imgFile.close();
    uint16_t img_w = 0, img_h = 0; if (get_jpeg_width_height(img_buf, imgSize, &img_w, &img_h) != 0) { free(img_buf); return "{\"hata\":\"JPEG boyutu okunamadi!\"}"; }
    uint8_t *rgb_buf = (uint8_t*)ps_calloc(img_w * img_h * 3, sizeof(uint8_t)); fmt2rgb888(img_buf, imgSize, PIXFORMAT_JPEG, rgb_buf); free(img_buf);
    int target_h = tflite_input->dims->data[1]; int target_w = tflite_input->dims->data[2]; int target_c = tflite_input->dims->data[3]; uint8_t *roi_buf = (uint8_t*)ps_malloc(target_w * target_h * 3); String okunan_deger = "";
    float rad = config_rotation * 0.0174532925f; float cos_a = cos(-rad); float sin_a = sin(-rad); float cx = img_w / 2.0f; float cy = img_h / 2.0f;
    DynamicJsonDocument responseDoc(32768); JsonArray detaylar = responseDoc.createNestedArray("detaylar");

    for (JsonObject roi : rois) {
        int rx = roi["x"]; int ry = roi["y"]; int rw = roi["w"]; int rh = roi["h"];
        for (int ty = 0; ty < target_h; ty++) {
            for (int tx = 0; tx < target_w; tx++) {
                float unrot_x = rx + (tx * rw) / (float)target_w; float unrot_y = ry + (ty * rh) / (float)target_h;
                float dx = unrot_x - cx; float dy = unrot_y - cy;
                int src_x = (int)round(cx + dx * cos_a - dy * sin_a); int src_y = (int)round(cy + dx * sin_a + dy * cos_a);
                if (src_x < 0) src_x = 0; if (src_y < 0) src_y = 0; if (src_x >= img_w) src_x = img_w - 1; if (src_y >= img_h) src_y = img_h - 1;
                int src_idx = (src_y * img_w + src_x) * 3; int dst_idx = (ty * target_w + tx) * 3;
                roi_buf[dst_idx] = rgb_buf[src_idx]; roi_buf[dst_idx + 1] = rgb_buf[src_idx + 1]; roi_buf[dst_idx + 2] = rgb_buf[src_idx + 2];
            }
        }
        for (int p = 0; p < target_w * target_h; p++) {
            if (target_c == 1) { 
                int gray = (roi_buf[p*3] + roi_buf[p*3+1] + roi_buf[p*3+2]) / 3;
                if (tflite_input->type == kTfLiteFloat32) tflite_input->data.f[p] = (float)gray / 255.0f; else if (tflite_input->type == kTfLiteInt8) tflite_input->data.int8[p] = (int8_t)(gray - 128); else if (tflite_input->type == kTfLiteUInt8) tflite_input->data.uint8[p] = gray;
            } else { 
                if (tflite_input->type == kTfLiteFloat32) { tflite_input->data.f[p*3] = (float)roi_buf[p*3] / 255.0f; tflite_input->data.f[p*3+1] = (float)roi_buf[p*3+1] / 255.0f; tflite_input->data.f[p*3+2] = (float)roi_buf[p*3+2] / 255.0f; } 
                else if (tflite_input->type == kTfLiteInt8) { tflite_input->data.int8[p*3] = (int8_t)((int)roi_buf[p*3] - 128); tflite_input->data.int8[p*3+1] = (int8_t)((int)roi_buf[p*3+1] - 128); tflite_input->data.int8[p*3+2] = (int8_t)((int)roi_buf[p*3+2] - 128); } 
                else if (tflite_input->type == kTfLiteUInt8) { tflite_input->data.uint8[p*3] = roi_buf[p*3]; tflite_input->data.uint8[p*3+1] = roi_buf[p*3+1]; tflite_input->data.uint8[p*3+2] = roi_buf[p*3+2]; }
            }
        }
        tflite_interpreter->Invoke(); int max_index = 0; float max_val = -9999.0;
        for (int c = 0; c < 11; c++) { float val = 0; if (tflite_output->type == kTfLiteFloat32) val = tflite_output->data.f[c]; else if (tflite_output->type == kTfLiteInt8) val = tflite_output->data.int8[c]; else if (tflite_output->type == kTfLiteUInt8) val = tflite_output->data.uint8[c]; if (val > max_val) { max_val = val; max_index = c; } }
        int yuzde = 0; if (tflite_output->type == kTfLiteInt8) yuzde = ((max_val + 128) * 100) / 255; else if (tflite_output->type == kTfLiteUInt8) yuzde = (max_val * 100) / 255; else yuzde = (int)(max_val * 100); if(yuzde > 100) yuzde = 100; if(yuzde < 0) yuzde = 0;
        JsonObject detay = detaylar.createNestedObject(); detay["img"] = getBmpBase64(roi_buf, target_w, target_h); detay["p"] = yuzde;
        if (max_index < 10) { detay["d"] = String(max_index); okunan_deger += String(max_index); } else { detay["d"] = "N"; }
    }
    free(roi_buf); free(rgb_buf);
    
    String tuketim = ""; config_lastValue = okunan_deger;
    if(config_faturaEndeks != "") { tuketim = String(okunan_deger.toInt() - config_faturaEndeks.toInt()); }
             
    responseDoc["deger"] = okunan_deger; responseDoc["tuketim"] = tuketim; responseDoc["hata"] = false;
    String finalJson; serializeJson(responseDoc, finalJson);

    checkAndSendWebhook(okunan_deger, tuketim, fileName);
    saveSistemConfig(); 
             
    struct tm timeinfo; String histTime = "Zaman Yok";
    if(getLocalTime(&timeinfo)){ char tBuff[50]; strftime(tBuff, sizeof(tBuff), "%d.%m.%Y %H:%M:%S", &timeinfo); histTime = String(tBuff); }
    File histFile = SD_MMC.open("/history.csv", FILE_APPEND);
    if(histFile){ histFile.print(histTime + "," + okunan_deger + "," + (tuketim == "" ? "-" : tuketim) + "," + config_faturaEndeks + "," + fileName + "\n"); histFile.close(); }
    
    File resFile = SD_MMC.open("/config/last_result.json", FILE_WRITE);
    if(resFile){ resFile.print(finalJson); resFile.close(); }

    return finalJson;
}

String askGemini(String fileName) {
    config_apiKey.trim(); if(config_apiKey == "") return "{\"hata\":\"API Anahtari Yok!\"}";
    addLog("AI Motoru: Gemini Flash Lite baslatiliyor...");
    File file = SD_MMC.open(fileName, FILE_READ); if(!file) return "{\"hata\":\"Goruntu dosyasi acilamadi!\"}";
    size_t size = file.size(); uint8_t *img_buf = (uint8_t*)ps_malloc(size); file.read(img_buf, size); file.close();
    String base64Img = base64::encode(img_buf, size); free(img_buf); 

    String prompt = "Sen uzman bir dogalgaz sayaci okuyucususun. Gonderdigim resimdeki dogalgaz sayacinin ana endeksini oku. "
                    "ONEMLI KURALLAR: 1. Sadece sayacin uzerindeki okunan tam sayiyi tespit et. "
                    "2. Bir onceki okunan deger: " + config_lastValue + ". Yeni okudugun deger bu sayidan kucuk olamaz. Eger oyle gorunuyorsa resmi analiz edip mantikli olani bul. "
                    "3. ASLA fazladan bir metin yazma. Sadece JSON dondur: { \\\"okunan_deger\\\": 16942, \\\"hata_var_mi\\\": false }";
    
    size_t payload_size = base64Img.length() + prompt.length() + 200; char *payload = (char*)ps_malloc(payload_size); if(!payload) return "{\"hata\":\"RAM Hatasi\"}";
    snprintf(payload, payload_size, "{\"contents\":[{\"parts\":[{\"text\":\"%s\"},{\"inline_data\":{\"mime_type\":\"image/jpeg\",\"data\":\"%s\"}}]}]}", prompt.c_str(), base64Img.c_str());
    base64Img = ""; WiFiClientSecure client; client.setInsecure(); HTTPClient https;
    String url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=" + config_apiKey;
    String resultJson = "{\"hata\":\"Bilinmeyen Hata\"}";

    if (https.begin(client, url)) {
      // YENİ: ZAMAN AŞIMI TOLERANSI 45 SANİYEYE UZATILDI
      https.setTimeout(45000); 
      https.addHeader("Content-Type", "application/json");
      
      int httpCode = 0;
      String response = "";
      
      for (int retry = 1; retry <= 3; retry++) {
          httpCode = https.POST((uint8_t*)payload, strlen(payload)); 
          if (httpCode == 200) {
              response = https.getString();
              break; 
          } else if (httpCode == 503 || httpCode == 500) {
              addLog("Google Sunuculari Mesgul (Hata " + String(httpCode) + "). 5 sn bekleniyor... (Deneme " + String(retry) + ")");
              delay(5000); 
          } else {
              response = https.getString();
              break; 
          }
      }
      free(payload);
      
      if (httpCode == 200) {
        StaticJsonDocument<2048> doc; deserializeJson(doc, response);
        String textResult = doc["candidates"][0]["content"]["parts"][0]["text"].as<String>();
        textResult.replace("```json", ""); textResult.replace("```", ""); textResult.trim();
        addLog("Gemini'nin Ham Yaniti: " + textResult);
        StaticJsonDocument<512> geminiDoc; DeserializationError err = deserializeJson(geminiDoc, textResult);
        if(!err) {
          String rakam = geminiDoc["okunan_deger"].as<String>(); bool hata = geminiDoc["hata_var_mi"] | false; String tuketim = "";
          if(!hata && rakam != "null" && rakam != "") {
             config_lastValue = rakam;
             if(config_faturaEndeks != "") { tuketim = String(rakam.toInt() - config_faturaEndeks.toInt()); }
             
             checkAndSendWebhook(rakam, tuketim, fileName); 
             saveSistemConfig(); 
             
             struct tm timeinfo; String histTime = "Zaman Yok";
             if(getLocalTime(&timeinfo)){ char tBuff[50]; strftime(tBuff, sizeof(tBuff), "%d.%m.%Y %H:%M:%S", &timeinfo); histTime = String(tBuff); }
             File histFile = SD_MMC.open("/history.csv", FILE_APPEND); 
             if(histFile){ histFile.print(histTime + "," + rakam + "," + (tuketim == "" ? "-" : tuketim) + "," + config_faturaEndeks + "," + fileName + "\n"); histFile.close(); }
             
             DynamicJsonDocument resDoc(1024); resDoc["deger"] = rakam; resDoc["tuketim"] = tuketim; resDoc["hata"] = hata; resDoc["zaman"] = histTime;
             serializeJson(resDoc, resultJson);
             File resFile = SD_MMC.open("/config/last_result.json", FILE_WRITE); if(resFile){ resFile.print(resultJson); resFile.close(); }
          }
        } else { addLog("HATA: Gemini format hatasi!"); resultJson = "{\"hata\":\"Gemini format hatasi.\"}"; }
      } else { addLog("HTTP Hatasi: " + String(httpCode)); resultJson = "{\"hata\":\"HTTP Kodu: " + String(httpCode) + "\"}"; }
      https.end();
    }
    return resultJson;
}