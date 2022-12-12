#include "RealTimeClock.h"
#include <Arduino.h>
#include <ESP32Time.h>
#include "SerialDebug.h"

RTC_DS3231 rtc;
ESP32Time Inrtc;

void RealTimeClock::Initialize(int UpdateDelay)
{
  UpdateFrequency = (UpdateDelay < 1000) ? 1000 / UpdateDelay : 0;
  if (!rtc.begin(&Wire))
  {
    Debug.print("[Clock] Couldn't find RTC! Used System Upload Time : ");
    Debug.print(__DATE__);
    Debug.print(" ");
    Debug.println(__TIME__);
    RtcBegin = false;
    now = DateTime(CompileTime.unixtime() + millis() / 1000);
  }
  else
  {
    if (rtc.lostPower())
    //if(true)
    {
      RtcLostPower = true;
      Debug.print("[Clock] RTC lost power, reset the time : ");
      Debug.print(__DATE__);
      Debug.print(" ");
      Debug.println(__TIME__);
      rtc.adjust(CompileTime);
    }

    // Load the time to give an initial time.
    for (int i = 0; i < 10; i++)
    {
      if (rtc.now().isValid())
      {
        now = rtc.now();
        CompileTime = DateTime(now.unixtime() - millis() / 1000);
        Debug.println("[Clock] RTC Start, now = " + DateTimeStamp());
        Inrtc.setTime(now.unixtime()); // Set esp internal rtc time (For SDCard modified time)
        return;
      }
    }
    RtcBegin = false;
    now = DateTime(CompileTime.unixtime() + millis() / 1000);
    Inrtc.setTime(now.unixtime());
    Debug.println("[Clock] Can't Load RTC Time, try to reset the RTC (turn off main power and install the RTC battary again)");
  };
}

void RealTimeClock::update()
{
  // If using now = rtc.now() directly in date-time string generating function,
  // the library may consume large amount of ROM and have high possibility to crash.
  // Update the date-time information in individual loop cycle will be more stable.

  // If RTC Begin faild in Initialize, assume no RTC install and use esp build in rtc.
  if (!RtcBegin)
  {
    now = DateTime(CompileTime.unixtime() + millis() / 1000);
    return;
  }

  if (tInvalid > UpdateFrequency)
  { // if reading RTC keep failling over 1 second, try to restart the RTC every 5 sec.
    if (tBeginFalse % (5 * UpdateFrequency) != 0)
    { // used build in rtc between checking rtc.begin()
      tBeginFalse++;
      now = DateTime(CompileTime.unixtime() + millis() / 1000);
      return;
    }
    else
    {
      if (!rtc.begin())
      {
        if (tBeginFalse == 0)
        { // Print error once.
          Debug.println("[Clock] RTC.begin() Return False");
        }
        tBeginFalse++;
        now = DateTime(CompileTime.unixtime() + millis() / 1000);
        return;
      }
      else
      {
        tBeginFalse = 0;
        tInvalid = 0;
        Debug.println("[Clock] RTC.begin() Return True");
      }
    }
  } // end rtc.begin() check

  DateTime NewTime = rtc.now();
  if (!NewTime.isValid())
  {
    tInvalid++;
    Debug.println("[Clock] RTC.now() Data Invalid");
    now = DateTime(CompileTime.unixtime() + millis() / 1000);
  } // end Check valid
  else if (NewTime == LastUpdate)
  {
    if (millis() - tLastUpdate > 1200)
    {
      if(!LostRTC)
      {
        LostRTC = true;
        CompileTime = DateTime(now.unixtime() - tLastUpdate);
        Debug.println("[Clock] RTC.now() Not Update");
      }
      tInvalid++;
      now = DateTime(CompileTime.unixtime() + millis() / 1000);
    }
  }
  else
  {
    if (tInvalid != 0)
    {
      Debug.println("[Clock] RTC OK");
    }
    now = NewTime;
    LastUpdate = NewTime;
    LostRTC = false;
    tLastUpdate = millis();
    tInvalid = 0;
  }
}

int RealTimeClock::NowSec()
{
  return now.unixtime();
}

String RealTimeClock::TimeStamp(String NowSet, String str)
{
  NowSet.toUpperCase();
  DateTime T;
  if (NowSet == "NOW")
  {
    T = now;
  }
  else if (NowSet == "SET")
  {
    T = UserSetTimeBuffer;
  }
  else
  {
    Debug.println("[Clock] TimeStamp print Error. T = now");
  }

  String Now = "";

  if (T.hour() < 10)
  {
    Now += "0";
  }
  Now += String(T.hour()) + str;
  if (now.minute() < 10)
  {
    Now += "0";
  }
  Now += String(T.minute()) + str;
  if (T.second() < 10)
  {
    Now += "0";
  }
  Now += String(T.second());

  return Now;
}

String RealTimeClock::DateStamp(String NowSet, String YMD, String str, int YearDigit)
{
  NowSet.toUpperCase();
  DateTime T;
  if (NowSet == "NOW")
  {
    T = now;
  }
  else if (NowSet == "SET")
  {
    T = UserSetTimeBuffer;
  }
  else
  {
    Debug.println("[Clock] TimeStamp print Error. T = now");
  }

  String Today = "";
  if (YMD == "DMY")
  {
    if (T.day() < 10)
    {
      Today += "0";
    }
    Today += String(T.day()) + str;
    if (T.month() < 10)
    {
      Today += "0";
    }
    Today += String(T.month()) + str + String(T.year()).substring(min(4 - YearDigit, 4));
  }
  else
  {
    Today = String(T.year()).substring(min(4 - YearDigit, 4)) + str;
    if (T.month() < 10)
    {
      Today += "0";
    }
    Today += String(T.month()) + str;
    if (T.day() < 10)
    {
      Today += "0";
    }
    Today += String(T.day());
  }
  return Today;
}

String RealTimeClock::TimeStamp()
{
  return TimeStamp("Now", ":");
}

String RealTimeClock::DateStamp(String str, int YearDigit)
{
  return DateStamp("now", "YMD", str, YearDigit);
}

String RealTimeClock::DateTimeStamp()
{
  return (DateStamp("now", "YMD", "/", 4) + " " + TimeStamp());
}

char RealTimeClock::RTC_State()
{
  if (!RtcBegin || tBeginFalse > UpdateFrequency * 300)
  {
    return '*';
  }
  else if (RtcLostPower || tInvalid > UpdateFrequency)
  {
    return '!';
  }
  else
  {
    return ' ';
  }
}