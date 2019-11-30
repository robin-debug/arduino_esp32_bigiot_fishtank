#include "DalyTask.h"

//0700A125,2200A025,0730B925,0730B925,0800B925,

DalyTask::DalyTask(String t)
{
    task = t;
}


bool DalyTask::addTask(String t)
{
    Serial.print("addTask ");
    Serial.println(t.c_str());
    Serial.println(task.c_str());        

    if(!checkFormat(t))
        return false;
    task+=t+",";
    Serial.println(task.c_str());
    return true;
}

bool DalyTask::delTask(String t)
{
    Serial.print("delTask ");
    Serial.println(t.c_str());
    Serial.println(task.c_str());

    if(!checkFormat(t))
        return false;
    t = t.substring(0,6);
    int a = task.indexOf(t);
    if(a==-1){
        Serial.println("del fail");
        return false;
    }
    task.remove(a,9);
    Serial.println(task.c_str());
    return true;
}

bool DalyTask::doTask(bool (*fn)(String))
{
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) {
        Serial.println("getLocalTime fail");
        return false;
    }
    
    //Serial.print("doTask ");
    //Serial.println(task.c_str());
    char t[64] = {0};
    strftime(t, 64, "%H%M", &timeinfo);
    //Serial.println(t); 
    int a=task.indexOf(t);
    if(a==-1){
        //Serial.println("no task");
        return false;//no task
    }
    String n = task.substring(a,a+8);
    //Serial.println(n.c_str());
    strftime(t, 64, "%d", &timeinfo);
    //Serial.println(t);
    String d=n.substring(6,8);
    //Serial.println(d.c_str());
    if(d==String(t)){
        //Serial.println("task done today");
        return false;//task done today
    }
    Serial.println("launch task now!");
    if(!fn(n)){
        Serial.println("task fail");
        return false;//task fail
    }
    String mid=n.substring(0,6)+t+',';
    //Serial.println(mid.c_str());
    String left=task.substring(0,a);
    //Serial.println(left.c_str());
    String right=task;
    right.remove(0,a+9);
    //Serial.println(right.c_str());
    task=left+mid+right;
    Serial.println(task.c_str());
    return true;    
}

bool DalyTask::checkFormat(String t)
{
    if(t.length() == 8 && -1==t.indexOf(','))
        return true;
    Serial.print("bad format ");
    Serial.println(t.c_str());
    return false;
}
