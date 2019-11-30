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

#include "DalyTask.h"
DalyTask tasks("0700A100,2200A000,0730B900,1230B900,1730B900,");

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 8 * 3600;
const int   daylightOffset_sec = 8 * 3600;

const int   LED_BUILTIN = 4;
//const int   PIN_OUTPUTA = 12;//noot boot,flash read err, 1000
const int   PIN_OUTPUTA = 2;
const int   PIN_OUTPUTB = 13;

BIGIOT bigiot;

void eventCallback(const int devid, const int comid, const char *comstr, const char *slave)
{
    // You can handle the commands issued by the platform here.
    Serial.printf(" device id:%d ,command id:%d command string:%s ,slave:%s\n", devid, comid, comstr, slave);
    if(comid == PLAY)
        comstr = "C1";//turn light on
    else if(comid == STOP)
        comstr = "C0";//trun light off
    else if(comid == UP)
        comstr = "cam";//take photo

    if(0==strncmp(comstr,"cam",3))
        cmdCam(slave, comstr+3);
    else if(0==strncmp(comstr,"time",4))
        cmdTime(slave, comstr+4);
    else if(0==strncmp(comstr,"light",5))
        cmdLight(slave, comstr+5);
    else if(0==strncmp(comstr,"task",4))
        cmdTask(slave, comstr+4);
    else
        cmdHelp(slave, comstr);
}

void cmdTask(const char *client, const char *comstr)
{
    Serial.print("cmdTask ");
    Serial.println(comstr);
    if(comstr[0]=='+')
        tasks.addTask(String(comstr+1));
    else if(comstr[0]=='-')
        tasks.delTask(String(comstr+1));
    Serial.println(tasks.task.c_str());
    bigiot.sayToClient(client,tasks.task.c_str());
}

void cmdCam(const char *client, const char *comstr)
{
    Serial.print("cmdCam ");
    Serial.println(comstr);
    framesize_t framesize = FRAMESIZE_VGA;
    if(0==strcmp(comstr,"2048"))
        framesize = FRAMESIZE_QXGA;//2040x1920
    else if(0==strcmp(comstr,"1600"))
        framesize = FRAMESIZE_UXGA;//1600x1200
    else if(0==strcmp(comstr,"1280"))
        framesize = FRAMESIZE_SXGA;//1280x960
    else if(0==strcmp(comstr,"1024"))
        framesize = FRAMESIZE_XGA;//1024x768
    else if(0==strcmp(comstr,"800"))
        framesize = FRAMESIZE_SVGA;//800x600
    setCam(framesize);
    uploadCam();         
    bigiot.sayToClient(client,"photo upload!");
}

void cmdTime(const char *client, const char *comstr)
{
    Serial.print("cmdTime ");
    Serial.println(comstr);
    String msg = "";
    if(0==strcmp(comstr,"0")){
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        msg += "sync time, ";
    }
    struct tm timeinfo;
    if(getLocalTime(&timeinfo)) {
        char buf[64] = {0};
        strftime(buf, 64, "%A, %B %d %Y %H:%M:%S", &timeinfo);
        msg+=buf;
    } else
        msg +="Failed to obtain time";
    Serial.println(msg.c_str());
    if(client)
        bigiot.sayToClient(client,msg.c_str());
}

void cmdLight(const char *client, const char *comstr)
{    
    Serial.print("cmdLight ");
    Serial.println(comstr);
    //len of comstr must is 2,eg:A0,B1,C9
    if(strlen(comstr)!=2)
        comstr="C9";//led blink
    String msg="cmdLight ok";
    if(comstr[0]=='A'){
        if(comstr[1]=='0')//relay,low to open
            digitalWrite(PIN_OUTPUTA,HIGH);
        else if(comstr[1]=='1')
            digitalWrite(PIN_OUTPUTA,LOW);
        else
            msg="bad value for A";
    }else if(comstr[0]=='B'){//fedder
        if(comstr[1]=='9'){
            digitalWrite(PIN_OUTPUTB,LOW);
            delay(500);
            digitalWrite(PIN_OUTPUTB,HIGH);          
        }else
            msg="bad value for B";
    }else if(comstr[0]=='C'){//led blink
        if(comstr[1]=='0')
            digitalWrite(LED_BUILTIN,LOW);
        else if(comstr[1]=='1')
            digitalWrite(LED_BUILTIN,HIGH);
        else if(comstr[1]=='9'){
            digitalWrite(LED_BUILTIN,HIGH);
            delay(500);
            digitalWrite(LED_BUILTIN,LOW);           
        }else
            msg="bad value for C";
    }else
        msg="bad pin";
    Serial.println(msg.c_str());
    if(client)
        bigiot.sayToClient(client,msg.c_str());
}

void cmdHelp(const char *client, const char *comstr)
{
    String msg = String("bad cmd ") + comstr + ",try cam or time or light or task";
    bigiot.sayToClient(client,msg.c_str());
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

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PIN_OUTPUTA, OUTPUT);
    pinMode(PIN_OUTPUTB, OUTPUT);
    cmdLight(NULL,"C1");
    digitalWrite(PIN_OUTPUTA,HIGH);//relay off
    digitalWrite(PIN_OUTPUTB,HIGH);//relay off
    
    initCam();

    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, passwd);

//    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
//        Serial.print("Connect ssid fail");
//        while (1);
//    }
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        cmdLight(NULL,"C9");
        Serial.print(".");
    }
    cmdLight(NULL,"C1");
    Serial.println("connected: OK");

    //init and get the time
    cmdTime(NULL,"0");

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
    cmdLight(NULL,"C0");
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

bool taskCallback(String t)//0700A125
{
    Serial.print("taskCallback ");
    Serial.print(t.c_str());
    t=t.substring(4,6);
    cmdLight(NULL,t.c_str());
    return true;
}

void loop()
{
    if (WiFi.status() == WL_CONNECTED) {
        //Wait for platform command release
        bigiot.handle();
    } else {
        static uint64_t last_wifi_check_time = 0;
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
        long now = millis();
        if(lastTime == 0 || now - lastTime > 60000) {
            uploadCam();
            lastTime = now;
        }
    }

    {
        static long lastTime = 0;
        long now = millis();
        if(lastTime == 0 || now - lastTime > 10000) {
            cmdTime(NULL,"time");
            lastTime = now;
        }
    }

    tasks.doTask(&taskCallback);//work with daly task
}
