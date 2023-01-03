#include <esp_camera.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// Camera MAC - 24:6F:28:B3:2E:7C
// Receiver MAC - A4:CF:12:60:F4:98
// Receiver U3 - 30:AE:A4:9B:AF:B8
camera_config_t config;
esp_now_peer_info_t peerInfo = {};
// uint8_t broadcastAddress[] = {0xA4, 0xCF, 0x12, 0x60, 0xF4, 0x98};
uint8_t broadcastAddress[] = {0x30, 0xAE, 0xA4, 0x9B, 0xAF, 0xB8};
uint8_t STX = { 0x01 };
uint8_t ETX = { 0x02 };
uint8_t PMK[] = { 0x01, 0x14, 0x93, 0x08, 0x27, 0x93, 0x12, 0x26, 0x91, 0x10, 0x14, 0x94, 0x03, 0x25, 0x93, 0x00 };
uint8_t LMK[] = { 0x01, 0x14, 0x93, 0x08, 0x27, 0x93, 0x12, 0x26, 0x91, 0x10, 0x14, 0x94, 0x03, 0x25, 0x93, 0x01 };

void send_data(uint8_t * data, uint8_t len) {
  if(data[0] == STX) {
    Serial.println("Sending STX");
  } else if(data[0] == ETX) {
    Serial.println("Sending ETX");
  }

  int count = 0;
  esp_err_t result = esp_now_send(broadcastAddress, data, len);
  while(result != ESP_OK && count < 10) {
    result = esp_now_send(broadcastAddress, data, len);
    count += 1;
  }
}

void take_picture() {
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;

  int64_t fr_start = esp_timer_get_time();

  fb = esp_camera_fb_get();
  if (!fb) {
      Serial.println("CAPTURE: failed to acquire frame");
      return;
  } else {
    Serial.println("Sending photo!");

    int count = 0;

    // STX
    send_data(&STX, 1);
    delay(50);

    // Image
    while(count * ESP_NOW_MAX_DATA_LEN < fb->len) {
      int l = fb->len - count * ESP_NOW_MAX_DATA_LEN;

      if(l > ESP_NOW_MAX_DATA_LEN) 
        l = ESP_NOW_MAX_DATA_LEN;

      send_data(fb->buf + (count * ESP_NOW_MAX_DATA_LEN), l);
      delay(50);

      count += 1;
    }

    // ETX 
    send_data(&ETX, 1);

    Serial.println("Total bytes: " + String(fb->len));
  }

  esp_camera_fb_return(fb);
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("ESPNowCamera started!");

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  // config.frame_size = FRAMESIZE_VGA;
  // config.frame_size = FRAMESIZE_HVGA;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 64;
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_LATEST;

  esp_camera_init(&config);

  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_15dBm);

  esp_now_deinit();
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  } else {
    Serial.println("ESP-NOW Initialized Successful");
    esp_now_set_pmk((uint8_t *)PMK);

    for (uint8_t i = 0; i < 16; i++) {
      peerInfo.lmk[i] = LMK[i];
    }

    peerInfo.channel = 0;
    peerInfo.encrypt = true;
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);

    esp_now_del_peer(peerInfo.peer_addr);

    if (esp_now_add_peer(&peerInfo) != ESP_OK){
      Serial.println("Failed to add peer");
      return;
    } else {
      Serial.println("Peer added");
    }
  }

  WiFi.scanDelete();
 
}

void loop() {
  take_picture();
  // delay(1000);
}
