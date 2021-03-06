# 智能鱼缸控制器 iot fish tank
<img src="https://raw.githubusercontent.com/robin-debug/arduino_esp32_bigiot_fishtank/master/data/tank.jpg">

## 简介
为了使现有鱼缸设备智能化，将定时任务自动执行，并能远程查/看/控。

本项目使用esp32-cam主板，arduino环境，物联网平台使用bigiot免费平台。
<img src="https://raw.githubusercontent.com/robin-debug/arduino_esp32_bigiot_fishtank/master/data/device.jpg">
## 特点
* 看（可视化），设备自动上传图像到iot平台，可远程查看鱼缸最新实况
* 查（数据上报），除图像外，设备还上传传感器/控制通道的数据
* 控（命令系统），借助微信和网站平台，可以通过命令行，控制设备

设备上传真实样张
<img src="https://raw.githubusercontent.com/robin-debug/arduino_esp32_bigiot_fishtank/master/data/cam.jpg">

## 硬件组成
* esp32-cam，这块板子包含esp32主控芯片，板载4M闪存/200M像素摄像头/读卡器/wifi/蓝牙
* 4路继电器模块，220伏10安，光耦隔离，高电位触发
<img src="https://raw.githubusercontent.com/robin-debug/arduino_esp32_bigiot_fishtank/master/data/esp32cam.png">

## 外接设备
继电器模块控制外部设备，包括
* 照明灯，继电器控制照明灯所在插板电源，实现智能插座，控制照明灯
* 喂食器，通过导出喂食器手动按钮的接线连接继电器，通过点动模拟手动喂食

<img src="https://raw.githubusercontent.com/robin-debug/arduino_esp32_bigiot_fishtank/master/data/link.jpg">

## Iot系统选择
之所以选择 bigiot.net 平台是因为:
* 有完整的anduino支持
* 支持图象数据上传
* 免费

## 存储系统
由于内置读卡器占用太多io,本项目使用SPIFFS存储配置文件。即使用内建闪存模拟文件系统。将wifi/iot参数/计划任务等保存在配置文件，在重启后恢复之前工作状态

## 监视狗
为防止因网络或其他原因导致系统锁死，系统设置监视狗定时器。当超时未响应，会触发系统重启恢复正常工作

## 定时任务
系统定时完成指定任务，如开关灯，启动喂食器等。为配合，系统在重启时向时间服务器同步系统时间(高精度时间模块没来及使用)。详情参见命令task

## 命令系统
iot平台支持微信，网站两种渠道向设备发送指令，但微信命令不能收到设备回复。
* cam[X|Y|分辨率] 摄象头，X设置最高帧率（受图像大小及网络限制约3秒一帧），Y十秒一帧，其他命令将恢复60秒一帧。分辨率可选2048/1600/1280/1024/800/640/320/160。本命令立即上传图像一张。
* time[X|0] 系统时间。X获取系统启动时间，0立即同步网络时间，其他返回当前系统时间
* light[X|A|B|C|D][0|1|9] 板载led/外接设备。X为板载led,A-D对应四路继电器。0关，1开，9点动
* task[+|-] 定时任务。无参数获取当前任务详情，+添加任务，-删除任务。任务格式HHMMXYDD,HHMM代表任务计划执行时间，XY代表执行通道和参数(参见light命令)，DD代表上次执行日期（未执行为00）
* reboot 强制重启
* value 立即上传各数据点数据，同时向命令行回复

## 参考
* bigiot.net
* iot sdk https://github.com/lewisxhe/ArduinoBIGIOT
* esp32-cam sample https://github.com/espressif/esp32-camera

## 其他
TODO:添加图片，完善文档

***
2019-nCov  中国加油,武汉加油 
