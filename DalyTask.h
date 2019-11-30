#if defined ESP32
#include <WiFi.h>
#elif defined ESP8266
#include <ESP8266WiFi.h>
#else
#error "Only support espressif esp32/8266 chip"
#endif
#include <String.h>

class DalyTask
{
public:
    DalyTask(String t);
    bool addTask(String t);
    bool delTask(String t);

    bool doTask(bool (*fn)(String));  
    bool checkFormat(String t);

    String task;
};
