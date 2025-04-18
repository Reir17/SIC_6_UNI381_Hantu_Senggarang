#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "esp_camera.h"
#include <ArduinoJson.h>
#include <ESP32Servo.h>


// Pilih model kamera
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// -----------------------------
// Ganti dengan milikmu
const char* ssid = "Tambelan";
const char* password = "bukacelane";

#define BOT_TOKEN "7670971898:AAHDvXoLei7QFRDEAvP4kxmyGTNyg4EbLrY"
#define CHAT_ID "5200137266" // Ganti dengan chat ID kamu
// -----------------------------

#define FLASH_LED_PIN 4
#define SERVO_PIN 13

Servo myservo;
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

bool flashState = LOW;
camera_fb_t *fb = NULL;

bool dataAvailable = false;
unsigned long bot_lasttime;
const unsigned long BOT_MTBS = 1000;

bool isMoreDataAvailable() {
  if (dataAvailable) {
    dataAvailable = false;
    return true;
  }
  return false;
}

byte *getNextBuffer() {
  return fb ? fb->buf : nullptr;
}

int getNextBufferLen() {
  return fb ? fb->len : 0;
}

// Dummy face recognition function
bool detectFace() {
  // Simulasi: Deteksi wajah selalu "true"
  return true;
}

// Kirim foto ke Telegram
void sendPhoto(String chat_id) {
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    bot.sendMessage(chat_id, "Camera capture failed", "");
    return;
  }
  dataAvailable = true;
  Serial.println("Sending photo...");
  bot.sendPhotoByBinary(chat_id, "image/jpeg", fb->len,
                        isMoreDataAvailable, nullptr,
                        getNextBuffer, getNextBufferLen);
  esp_camera_fb_return(fb);
  Serial.println("Photo sent!");
}

// Kontrol perintah dari Telegram
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;
    if (from_name == "") from_name = "Guest";

    if (text == "/start") {
      String welcome = "Hello " + from_name + ".\n";
      welcome += "/photo - Ambil foto dari ESP32-CAM\n";
      welcome += "/flash - Nyalakan/matikan lampu flash\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }

    if (text == "/flash") {
      flashState = !flashState;
      digitalWrite(FLASH_LED_PIN, flashState);
    }

    if (text == "/photo") {
      if (detectFace()) {
        sendPhoto(chat_id);
        myservo.write(90); // Buka pintu
        delay(2000);
        myservo.write(0); // Tutup pintu
      } else {
        sendPhoto(chat_id); // Tidak dikenal, tetap kirim
      }
    }
  }
}

// Setup kamera
bool setupCamera() {
  camera_config_t config;
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

  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  return err == ESP_OK;
}

void setup() {
  Serial.begin(115200);
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, flashState);

  myservo.attach(SERVO_PIN);
  myservo.write(0); // posisi awal servo

  if (!setupCamera()) {
    Serial.println("Camera init failed");
    while (true) delay(1000);
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  while (time(nullptr) < 24 * 3600) {
    delay(100);
  }

  bot.longPoll = 60;
}

void loop() {
  if (millis() - bot_lasttime > BOT_MTBS) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    bot_lasttime = millis();
  }
}
