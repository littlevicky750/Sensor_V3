#ifndef OLED_H
#define OLED_H
#include <U8g2lib.h>
#include <Wire.h>
#include "IMU901.h"
#include "RealTimeClock.h"



class OLED
{
private:
    int DotCount = 0;
    int Count = 0;
    uint8_t I2C_Add = 0x3C;
    void DrawH(IMU901 &imu);
    void DrawV(IMU901 &imu);
    void TestAddress();
    bool isU8G2Begin = false;

public:
    String *ConnectShow;
    String *SDShow;
    int *SDLastCheck;
    int *Bat;
    RealTimeClock *ClockShow;
    int Mode_Clock;
    byte Page = 0;


    void Initialize();
    void Clear();
    void TurnOff();
    void ShowLowPower();
    void Update(IMU901 &imu);

};

#endif