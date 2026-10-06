// --- KAMERA VE FLAŞ DONANIM YÖNETİM MODÜLÜ ---

#define FLASH_LED_PIN 4
const int pwmChannel = 0;
const int pwmFreq = 5000;
const int pwmResolution = 8;

#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

void applyCameraSettings(framesize_t size) {
  sensor_t * s = esp_camera_sensor_get();
  if (s) { 
    s->set_framesize(s, size); 
    s->set_quality(s, config_quality); 
  }
}

void initKamera() {
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(FLASH_LED_PIN, pwmChannel);
  ledcWrite(pwmChannel, 0);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_1; 
  config.ledc_timer = LEDC_TIMER_1;
  config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM; config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM; config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM; config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM; config.pin_vsync = VSYNC_GPIO_NUM; config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM; config.pin_sscb_scl = SIOC_GPIO_NUM; config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM; 
  config.xclk_freq_hz = 20000000; 
  config.pixel_format = PIXFORMAT_JPEG;
  
  config.frame_size = FRAMESIZE_VGA; // Kalibrasyon ve TFLite için kalıcı 640x480 (VGA) çözünürlüğü!
  config.jpeg_quality = config_quality; 
  config.fb_count = 2; 

  esp_err_t err = esp_camera_init(&config);
  if (err == ESP_OK) { 
    addLog("Kamera VGA donaniminda basariyla baslatildi."); 
  } else {
    addLog("HATA: Kamera baslatilamadi!");
  }
}