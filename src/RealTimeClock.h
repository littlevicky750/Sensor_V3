#ifndef RealTimeClock_h
#define RealTimeClock_h
#include <Arduino.h>
#include <RTClib.h>

class RealTimeClock
{
public:
  void Initialize(int UpdateDelay);
  String TimeStamp(String NowSet, String str);
  String TimeStamp();
  String DateStamp(String NowSet, String YMD, String str, int YearDigit);
  String DateStamp(String str, int YearDigit);
  String DateTimeStamp();
  char RTC_State(); 
  // return * : RTC Failed, used built in. ! : RTC Lost Power
  void update();
  int NowSec();
  int tInvalid = 0; // Read Data invalid count
  DateTime now;

private:
  DateTime CompileTime = DateTime(F(__DATE__), F(__TIME__));
  DateTime UserSetTimeBuffer;
  DateTime LastUpdate;
  bool RtcBegin = true;
  bool RtcLostPower = false;
  bool LostRTC = false;
  int tLastUpdate = 0;
  int tBeginFalse = 0;
  int UpdateFrequency = 10;
};
#endif
