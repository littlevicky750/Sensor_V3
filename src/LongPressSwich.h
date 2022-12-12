#ifndef LongPressSwich_H
#define LongPressSwich_H

#include <Arduino.h>
#include "OLED.h"
#include "SDCard.h"
#include "SerialDebug.h"
RTC_DATA_ATTR int bootCount = -1;

class LongPressSwich
{
private:
    int LEDFlashCount = 0;
    int OffClock = 0;
    byte LEDPin;
    byte SWPin;
    byte ButPin;

public:
    SDCard *pSD;
    void On(gpio_num_t WakeUpPin, byte Swich_Pin, byte LED_Pin_B, byte LED_Pin_Y, byte Bat_Pin)
    {
        pinMode(LED_Pin_B, OUTPUT);
        pinMode(LED_Pin_Y, OUTPUT);
        digitalWrite(LED_Pin_B, HIGH);
        esp_sleep_enable_ext0_wakeup(WakeUpPin, 0);
        bootCount++;
        // Begine from sleeping
        if (bootCount == 0)
        {
            esp_deep_sleep_start();
        }

        // Set Pin
        LEDPin = LED_Pin_B;
        ButPin = WakeUpPin;
        SWPin = Swich_Pin;
        pinMode(ButPin, INPUT);
        pinMode(Bat_Pin, INPUT);
        pinMode(SWPin, OUTPUT);
        digitalWrite(SWPin, HIGH);
        // Battary Test
        OLED oledb;
        /*
        if (analogRead(Bat_Pin) < 2740)
        {
            oledb.ShowLowPower();
            digitalWrite(LED_Pin_Y, HIGH);
            while(digitalRead(ButPin) == 0);
            digitalWrite(LED_Pin_Y,LOW);
            digitalWrite(LED_Pin_B,LOW);
            esp_deep_sleep_start();
        }
        */
        oledb.Clear();
        // Detect 3 min long press
        while (millis() < 3000 && digitalRead(ButPin) == 0)
        {
            delay(100);
        }
        digitalWrite(LEDPin, LOW);
        if (millis() < 3000)
        {
            digitalWrite(SWPin, LOW);
            esp_deep_sleep_start();
        }
        Serial.setRxBufferSize(256);
        Serial.begin(115200);
    }

    void Off_Clock_Start()
    {
        OffClock = millis();
    }

    void Off_Clock_Stop()
    {
        OffClock = 0;
    }

    void Off_Clock_Check()
    {
        if (OffClock == 0)
        {
            return;
        }
        if (millis() - OffClock > 3000)
        {
            cli();
            Debug.println("Function Off");
            if (pSD)
            {
                String T = "";
                pSD->Save("", T);
            }
            OLED oledb;
            oledb.TurnOff();
            detachInterrupt(digitalPinToInterrupt(ButPin));
            while (digitalRead(ButPin) == 0)
            digitalWrite(SWPin, LOW);
            digitalWrite(LEDPin, LOW);
            Serial.println("Sleep");
            esp_deep_sleep_start();
        }
        else
        {
            digitalWrite(LEDPin, HIGH);
        }
    }
};
#endif