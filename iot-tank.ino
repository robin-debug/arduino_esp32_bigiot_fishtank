#if defined ESP32
#include <WiFi.h>
#elif defined ESP8266
#include <ESP8266WiFi.h>
#else
#error "Only support espressif esp32/8266 chip"
#endif

#define DEBUG_BIGIOT_PORT Serial
#include "bigiot.h"

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"
#include "esp_camera.h"

#define WIFI_TIMEOUT 30000

#include "config.h"

BIGIOT bigiot;

void eventCallback(const int devid, const int comid, const char *comstr, const char *slave)
{
    // You can handle the commands issued by the platform here.
    Serial.printf(" device id:%d ,command id:%d command string:%s ,slave:%s\n", devid, comid, comstr, slave);
    if(0==strncmp(comstr,"cam",3))
        cmdCam(slave, comstr);
}

void cmdCam(const char *client, const char *comstr)
{
    framesize_t framesize = FRAMESIZE_VGA;
    if(0==strcmp(comstr,"cam2048"))
        framesize = FRAMESIZE_QXGA;
    else if(0==strcmp(comstr,"cam1600"))
        framesize = FRAMESIZE_UXGA;
    else if(0==strcmp(comstr,"cam1280"))
        framesize = FRAMESIZE_SXGA;
    else if(0==strcmp(comstr,"cam1024"))
        framesize = FRAMESIZE_XGA;
    else if(0==strcmp(comstr,"cam800"))
        framesize = FRAMESIZE_SVGA;
        uploadCam();         
    bigiot.sayToClient(client,"photo upload!");
}

void disconnectCallback(BIGIOT &obj)
{
    // When the device is disconnected to the platform, you can handle your peripherals here
    Serial.print(obj.deviceName());
    Serial.println("  disconnect");
}

void connectCallback(BIGIOT &obj)
{
    // When the device is connected to the platform, you can preprocess your peripherals here
    Serial.print(obj.deviceName());
    Serial.println("  connect");
}

void setCam(framesize_t framesize)
{
    sensor_t *s = esp_camera_sensor_get();
    s->set_framesize(s, framesize);
    //qqvga 160x120
    //qvga  320x240
    //vga   640x480
    //svga  800x600
    //xga   1024x768
    //uxga  1600x1200
}

void initCam()
{
    Serial.println("init camera");
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
    //init with high specs to pre-allocate larger buffers
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  
    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
      Serial.printf("Camera init failed with error 0x%x", err);
      while (1);
    }
  
    //drop down frame size for higher initial frame rate
    setCam(FRAMESIZE_SVGA);
}

void setup()
{
    Serial.begin(115200);
    delay(100);

    initCam();

    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, passwd);

    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.print("Connect ssid fail");
        while (1);
    }
    Serial.println("connected: OK");

    //Regist platform command event hander
    bigiot.eventAttach(eventCallback);

    //Regist device disconnect hander
    bigiot.disconnectAttack(disconnectCallback);

    //Regist device connect hander
    bigiot.connectAttack(connectCallback);

    // Login to bigiot.net
    if (!bigiot.login(id, apikey, usrkey)) {
        Serial.println("Login fail");
        while (1);
    }
    Serial.println("Connected to BIGIOT");
}

void uploadCam()
{
    Serial.println("cam");
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.printf("Camera capture failed");
        return;
    }
    Serial.printf("image len=%d,%d x %d\n", fb->len, fb->width, fb->height);
    int len = fb->len;
    if(len > 100 *1024 *1024)
      len = 100 * 1024 * 1024;
    if (!bigiot.uploadPhoto( picId, "jpg", "cam", (uint8_t *)fb->buf, len))
        Serial.println("Upload error");
    else
        Serial.println("Upload Success");
    esp_camera_fb_return(fb);    
}

void loop()
{
    static uint64_t last_wifi_check_time = 0;

    if (WiFi.status() == WL_CONNECTED) {
        //Wait for platform command release
        bigiot.handle();
    } else {
        uint64_t now = millis();
        // Wifi disconnection reconnection mechanism
        if (now - last_wifi_check_time > WIFI_TIMEOUT) {
            Serial.println("WiFi connection lost. Reconnecting...");
            WiFi.reconnect();
            last_wifi_check_time = now;
        }
    }

    {
        //upload image every 60s
        static long lastTime = 0;
        if(lastTime == 0 || millis() - lastTime > 60000) {
            uploadCam();
            lastTime = millis();
        }
    }
}
