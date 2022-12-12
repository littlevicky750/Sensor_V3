#ifndef LEDFlash_H
#define LEDFlash_H
#include <Arduino.h>
#include "SerialDebug.h"
#define LED_NUM 2

class LEDFlash
{
private:
    byte Channel[LED_NUM] = {0};
    byte Count[LED_NUM] = {0};
    byte Find(byte IO)
    {
        int i;
        for (i = 0; i < LED_NUM; i++)
        {
            if (Channel[i] == 0)
            {
                Channel[i] = IO;
                Count[i] = 0;
                break;
            }
            if (Channel[i] == IO)
            {
                break;
            }
        }
        if (i == LED_NUM)
        {
            Debug.println("[Error] LED Number Error. Check LED_NUM.");
        }
        return i;
    }

    void Blink(byte i, bool Frequency)
    {
        for (int j = 0; j < i; j++)
        {
            if (Count[j] == 1)
            {
                Count[i] = -1;
                break;
            }
        }
        Count[i]++;
        Count[i] %= (Frequency) ? 2 : 11;
        if (Count[i] == 1)
        {
            digitalWrite(Channel[i], HIGH);
        }
        else
        {
            digitalWrite(Channel[i], LOW);
        }
    }

public:
    void Set(byte pin, bool value[3])
    {
        int i = Find(pin);
        if (i < LED_NUM)
        {
            if (value[0])
            {
                Count[i] = 0;
                digitalWrite(pin, HIGH);
            }
            else if (value[1])
            {
                Blink(i, HIGH);
            }
            else if (value[2])
            {
                Blink(i, LOW);
            }
            else
            {
                Count[i] = 0;
                digitalWrite(pin, LOW);
            }
        }
    }
};

#endif