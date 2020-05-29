#if defined ESP32
#include <WiFi.h>
#elif defined ESP8266
#include <ESP8266WiFi.h>
#else
#error "Only support espressif esp32/8266 chip"
#endif

#include "SPIFFS.h"

//http://www.bigiot.net
//https://github.com/bigiot/bigiotArduino
//https://github.com/bigiot/Arduino_BIGIOT
//#define DEBUG_BIGIOT_PORT Serial
#include "bigiot.h"

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"
#include "esp_camera.h"

#define WIFI_TIMEOUT 30000

#include "config.h"
#include <ArduinoJson.h>

#include "DalyTask.h"
DalyTask tasks("");

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 8 * 3600;
const int daylightOffset_sec = 0; //8 * 3600;

const int LED_BUILTIN = 4;
const int ps[] = {2, 14, 15, 13, -1}; //12 not boot
char bootTime[64] = "";

long CAM_SLEEP = 300; //default 300s

BIGIOT bigiot;

const int wdtTimeout = 90000; //90s,time in ms to trigger the watchdog
hw_timer_t *timer = NULL;

void IRAM_ATTR resetModule()
{
    ets_printf("reboot\n");
    esp_restart();
}

/*
void cmdValue(const char *client, const char *comstr)
{
    String msg="cmdValue ",n;
    for(int i=0;;i++){
        if(ps[i]==-1) break;
        n=String(digitalRead(ps[i]));
        if(i==0)
            bigiot.upload(valueId,n.c_str());
        msg+=String(ps[i])+":"+n+",";
    }
    n=String(digitalRead(LED_BUILTIN));
    msg+=String(LED_BUILTIN)+":"+n;
    
    Serial.println(msg.c_str());
    if(client)
        bigiot.sayToClient(client,msg.c_str());
}
*/

void cmdReboot(const char *client, const char *comstr)
{
    Serial.println("cmdReboot");
    if (client)
        bigiot.sayToClient(client, "reboot now");
    esp_restart();
}

void cmdTask(const char *client, const char *comstr)
{
    Serial.print("cmdTask ");
    Serial.println(comstr);
    if (comstr[0] == '+')
    {
        tasks.addTask(String(comstr + 1));
        SaveCfg();
    }
    else if (comstr[0] == '-')
    {
        tasks.delTask(String(comstr + 1));
        SaveCfg();
    }
    Serial.println(tasks.task.c_str());
    if (client)
    {
        String t = tasks.task;
        t.replace(",", "<br>\n");
        bigiot.sayToClient(client, t.c_str());
    }
}

void cmdCam(const char *client, const char *comstr)
{
    Serial.print("cmdCam ");
    Serial.println(comstr);
    if (0 == strcmp(comstr, "X"))
        CAM_SLEEP = 1; //1s
    else if (0 == strcmp(comstr, "Y"))
        CAM_SLEEP = 10; //10s
    else
        CAM_SLEEP = 300; //300s

    framesize_t framesize = FRAMESIZE_QQVGA; //160x120
    if (0 == strcmp(comstr, "2048"))
        framesize = FRAMESIZE_QXGA; //2048*1536
    else if (0 == strcmp(comstr, "1600"))
        framesize = FRAMESIZE_UXGA; //1600x1200
    else if (0 == strcmp(comstr, "1280"))
        framesize = FRAMESIZE_SXGA; //1280x1024
    else if (0 == strcmp(comstr, "1024"))
        framesize = FRAMESIZE_XGA; //1024x768
    else if (0 == strcmp(comstr, "800"))
        framesize = FRAMESIZE_SVGA; //800x600
    else if (0 == strcmp(comstr, "640"))
        framesize = FRAMESIZE_VGA; //640x480
    else if (0 == strcmp(comstr, "320"))
        framesize = FRAMESIZE_QVGA; //320x240
    if (framesize != FRAMESIZE_QQVGA)
    {
        setCam(framesize);
        uploadCam(); //第一次没效果
    }
    bool ok = uploadCam();
    if (client)
        bigiot.sayToClient(client, ok ? "photo upload!" : "photo fail!");
}

void cmdTime(const char *client, const char *comstr)
{
    Serial.print("cmdTime ");
    Serial.println(comstr);
    String msg = "";
    if (0 == strcmp(comstr, "X"))
        msg = String("boot:") + bootTime;
    else
    {
        if (0 == strcmp(comstr, "0"))
        {
            configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
            msg += "sync time, ";
        }
        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            char buf[64] = {0};
            strftime(buf, 64, "%A, %B %d %Y %H:%M:%S", &timeinfo);
            if (bootTime[0] == 0)
                strcpy(bootTime, buf);
            msg += buf;
        }
        else
            msg += "Failed to obtain time";
    }
    Serial.println(msg.c_str());
    if (client)
        bigiot.sayToClient(client, msg.c_str());
}

void cmdLight(const char *client, const char *comstr)
{
    Serial.print("cmdLight ");
    Serial.println(comstr);
    //len of comstr must is 2,eg:A0,B1,C9
    if (strlen(comstr) != 2)
        comstr = "X9"; //led blink
    String msg = "cmdLight ok";
    if (comstr[0] == 'X')
    { //led
        if (comstr[1] == '0')
        {
            digitalWrite(LED_BUILTIN, LOW);
            SaveCfg();
        }
        else if (comstr[1] == '1')
        {
            digitalWrite(LED_BUILTIN, HIGH);
            SaveCfg();
        }
        else if (comstr[1] == '9')
        {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
        }
        else
            msg = String("bad value for ") + comstr[0];
    }
    else if (comstr[0] >= 'A' && comstr[0] <= 'D')
    {
        int port = ps[comstr[0] - 'A'];
        msg += port;
        if (comstr[1] == '0')
        {
            digitalWrite(port, HIGH); //relay off
            SaveCfg();
        }
        else if (comstr[1] == '1')
        {
            digitalWrite(port, LOW); //relay on
            SaveCfg();
        }
        else if (comstr[1] == '9')
        {
            digitalWrite(port, LOW); //relay on
            delay(100);
            digitalWrite(port, HIGH); //relay off
        }
        else
            msg = "bad value";
    }
    else
        msg = "bad pin";
    Serial.println(msg.c_str());
    if (client)
        bigiot.sayToClient(client, msg.c_str());
}

void cmdHelp(const char *client, const char *comstr)
{
    if (!client)
        return;
    //String msg = String("bad cmd ") + comstr + ",try cam/time/light/task/value/reboot";
    String msg = String("bad cmd ") + comstr + ",try cam/time/light/task/reboot";
    bigiot.sayToClient(client, msg.c_str());
}

void eventCallback(const int devid, const int comid, const char *comstr, const char *slave)
{
    // You can handle the commands issued by the platform here.
    Serial.printf(" device id:%d ,command id:%d command string:%s ,slave:%s\n", devid, comid, comstr, slave);
    if (comid == PLAY)
        comstr = "lightX9"; //led blink

    if (0 == strncmp(comstr, "cam", 3))
        cmdCam(slave, comstr + 3);
    else if (0 == strncmp(comstr, "time", 4))
        cmdTime(slave, comstr + 4);
    else if (0 == strncmp(comstr, "light", 5))
    {
        cmdLight(slave, comstr + 5);
        //cmdValue(slave,"");//upload values
    }
    else if (0 == strncmp(comstr, "task", 4))
        cmdTask(slave, comstr + 4);
    else if (0 == strncmp(comstr, "reboot", 6))
        cmdReboot(slave, comstr + 6);
    //else if(0==strncmp(comstr,"value",5))
    //    cmdValue(slave,comstr+5);
    else
        cmdHelp(slave, comstr);
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
    config.jpeg_quality = 7;
    config.fb_count = 2;

    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("Camera init failed with error 0x%x", err);
        //while (1);
    }

    //drop down frame size for higher initial frame rate
    setCam(FRAMESIZE_VGA);
}

StaticJsonDocument<4096> cfg;
bool LoadCfg()
{
    File file = SPIFFS.open(cfgFileName);
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return false;
    }
    String txt;
    while (file.available())
    {
        char c = file.read();
        txt += c;
    }
    file.close();
    Serial.println("File Content:");
    Serial.println(txt.c_str());

    cfg.clear();
    DeserializationError error = deserializeJson(cfg, txt);
    if (error)
    {
        Serial.printf("[%d] DeserializationError code:%d \n", __LINE__, error);
        return false;
    }
    tasks.task = String((const char *)cfg["task"]);

    char light[16] = {0};
    strcpy(light, cfg["light"]);
    for (int i = 0;; i++)
    {
        if (ps[i] == -1 || light[i] == 0)
            break;
        digitalWrite(ps[i], light[i] == '1' ? LOW : HIGH);
    }
    return true;
}

bool SaveCfg()
{
    cfg.clear();
    cfg["task"] = tasks.task.c_str();
    char light[16] = "00000";
    for (int i = 0;; i++)
    {
        if (ps[i] == -1)
            break;
        if (digitalRead(ps[i]) == LOW)
            light[i] = '1';
    }
    cfg["light"] = light;

    String txt;
    serializeJson(cfg, txt);
    Serial.println("File Content:");
    Serial.println(txt.c_str());

    File file = SPIFFS.open(cfgFileName, FILE_WRITE);
    if (!file)
    {
        Serial.println("Failed to open file for write");
        return false;
    }
    file.write((const uint8_t *)txt.c_str(), txt.length());
    file.close();
    return true;
}

void setup()
{
    Serial.begin(115200);
    delay(100);
    pinMode(LED_BUILTIN, OUTPUT);
    for (int i = 0;; i++)
    {
        if (ps[i] == -1)
            break;
        pinMode(ps[i], OUTPUT);
        digitalWrite(ps[i], HIGH); //relay off
    }
    cmdLight(NULL, "X9");

    initCam();

    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        while (1)
        {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(500);
            digitalWrite(LED_BUILTIN, LOW);
            Serial.print(".");
            delay(500);
        }
    }
    LoadCfg();

    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, passwd);

    while (WiFi.status() != WL_CONNECTED)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        Serial.print(".");
        delay(500);
    }
    Serial.println("connected: OK");

    //Regist platform command event hander
    bigiot.eventAttach(eventCallback);

    // Login to bigiot.net
    if (!bigiot.login(id, apikey, usrkey))
    {
        Serial.println("Login fail");
        while (1)
        {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(500);
            digitalWrite(LED_BUILTIN, LOW);
            Serial.print(".");
            delay(500);
        }
    }
    Serial.println("Connected to BIGIOT");

    //init and get the time
    cmdTime(NULL, "0");

    timer = timerBegin(0, 80, true);                  //timer 0, div 80
    timerAttachInterrupt(timer, &resetModule, true);  //attach callback
    timerAlarmWrite(timer, wdtTimeout * 1000, false); //set time in us
    timerAlarmEnable(timer);                          //enable interrupt

    cmdLight(NULL, "X9");
}

bool uploadCam()
{
    const char *id = picId;
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        static int lastWeek = 0;
        static int lastDay = 0;
        static int lastHour = 0;
        if (lastWeek != timeinfo.tm_wday && timeinfo.tm_hour == 12) //在12点执行
        {
            lastWeek = timeinfo.tm_wday;
            id = picIdWeek;
        }
        else if (lastDay != timeinfo.tm_mday && timeinfo.tm_hour == 12) //在12点执行
        {
            lastDay = timeinfo.tm_mday;
            id = picIdDay;
        }
        else if (lastHour != timeinfo.tm_hour)
        {
            lastHour = timeinfo.tm_hour;
            id = picIdHour;
        }
    }
    Serial.println("cam");
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb)
    {
        Serial.printf("Camera capture failed");
        return false;
    }
    Serial.printf("image len=%d,%d x %d\n", fb->len, fb->width, fb->height);
    int len = fb->len;
    if (len > 90 * 1024)
        len = 90 * 1024;
    bool ok = bigiot.uploadPhoto(id, "jpg", "cam", (uint8_t *)fb->buf, len);
    Serial.println(ok ? "Upload Success" : "Upload error");
    esp_camera_fb_return(fb);
    return ok;
}

bool taskCallback(String t) //0700A125
{
    Serial.print("taskCallback ");
    Serial.print(t.c_str());
    t = t.substring(4, 6);
    cmdLight(NULL, t.c_str());
    return true;
}

void loop()
{
    timerWrite(timer, 0); //reset timer (feed watchdog),heartbeat

    if (WiFi.status() == WL_CONNECTED)
    {
        //Wait for platform command release
        bigiot.handle();
    }
    else
    {
        static uint64_t last_wifi_check_time = 0;
        uint64_t now = millis();
        // Wifi disconnection reconnection mechanism
        if (now - last_wifi_check_time > WIFI_TIMEOUT)
        {
            Serial.println("WiFi connection lost. Reconnecting...");
            WiFi.reconnect();
            last_wifi_check_time = now;
        }
    }

    {
        //upload image every 300s
        static long lastTime = 0;
        long now = millis();
        if (lastTime == 0 || now - lastTime > CAM_SLEEP * 1000)
        {
            uploadCam();
            //cmdValue(NULL,"");
            //cmdTime(NULL,"");
            lastTime = now;
        }
    }

    tasks.doTask(&taskCallback); //work with daly task
}
