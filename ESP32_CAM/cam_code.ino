#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>

// ================== WIFI ==================
const char* ssid     = "POCO M6 Pro 5g";
const char* password = "chinmay12";

// ================== FLASH ==================
#define FLASH_LED_PIN 4  // AI THINKER FLASH LED

// ================== CAMERA PINS ==================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5

#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ============= SERVERS =============
WebServer server(80);        // UI /on /off
WiFiServer streamServer(82); // MJPEG Streaming

bool camActive = false;
TaskHandle_t streamTaskHandle;

// *************************************************************
// CAMERA CONFIGURATION (HIGH QUALITY + LOW LAG)
// *************************************************************
void initCamConfig(camera_config_t &config)
{
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk  = XCLK_GPIO_NUM;
  config.pin_pclk  = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href  = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn  = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;
  config.pixel_format  = PIXFORMAT_JPEG;

  // *********** HIGH QUALITY ***********
  config.frame_size   = FRAMESIZE_VGA;   // 640x480 — clear and smooth
  config.jpeg_quality = 8;               // sharper
  config.fb_count     = 1;               // stable for big frames
}

// *************************************************************
// START CAMERA
// *************************************************************
void camOn()
{
  if (camActive) return;

  camera_config_t config;
  initCamConfig(config);

  esp_err_t err = esp_camera_init(&config);
  if (err == ESP_OK)
  {
    camActive = true;
    digitalWrite(FLASH_LED_PIN, HIGH);
    Serial.println("CAMERA STARTED");
  }
  else
  {
    Serial.print("Camera init failed: 0x");
    Serial.println(err, HEX);
  }
}

// *************************************************************
// STOP CAMERA
// *************************************************************
void camOff()
{
  if (!camActive) return;
  esp_camera_deinit();
  camActive = false;
  digitalWrite(FLASH_LED_PIN, LOW);
  Serial.println("CAMERA STOPPED");
}

// *************************************************************
// ** MJPEG STREAMING TASK – CORE 0 **
// *************************************************************
void streamTask(void *parameter)
{
  streamServer.begin();
  Serial.println("MJPEG streaming active on :82 (Core 0)");

  while (true)
  {
    WiFiClient client = streamServer.available();
    if (!client)
    {
      vTaskDelay(50 / portTICK_PERIOD_MS);
      continue;
    }

    if (!camActive)
    {
      camOn();
      vTaskDelay(200 / portTICK_PERIOD_MS);
    }

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
    client.println();

    while (client.connected() && camActive)
    {
      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb)
      {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        continue;
      }

      client.println("--frame");
      client.println("Content-Type: image/jpeg");
      client.println("Content-Length: " + String(fb->len));
      client.println();
      client.write(fb->buf, fb->len);
      client.println();

      esp_camera_fb_return(fb);

      // ⭐ IMPORTANT: prevents WiFi choke
      vTaskDelay(35 / portTICK_PERIOD_MS);
    }

    client.stop();
  }
}

// *************************************************************
// UI ENDPOINT HANDLERS
// *************************************************************
void handleRoot()
{
  server.send(200, "text/plain",
    "ESP32-CAM READY\n"
    "/on  -> camera on\n"
    "/off -> camera off\n"
    "Stream at http://CAM_IP:82");
}

void handleOn()
{
  camOn();
  server.send(200, "text/plain", "CAMERA ON");
}

void handleOff()
{
  camOff();
  server.send(200, "text/plain", "CAMERA OFF");
}

// *************************************************************
// SETUP – CORE 1
// *************************************************************
void setup()
{
  Serial.begin(115200);

  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.print("\nCAM IP: ");
  Serial.println(WiFi.localIP());

  // UI endpoints
  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.begin();
  Serial.println("HTTP UI Ready :80");

  // START STREAMING TASK ON **CORE 0**
  xTaskCreatePinnedToCore(
    streamTask,
    "streamTask",
    10000,
    NULL,
    1,
    &streamTaskHandle,
    0
  );
}

// *************************************************************
// LOOP – CORE 1
// *************************************************************
void loop()
{
  server.handleClient();
}
