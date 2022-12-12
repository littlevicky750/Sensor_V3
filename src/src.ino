#define S3

#ifdef S3
const gpio_num_t Pin_Button_Wakeup = GPIO_NUM_0;
const byte Pin_SwichEN = 1;
const byte Pin_Button0 = Pin_Button_Wakeup;
const byte Pin_Button1 = 7;
const byte Pin_Button2 = 6;
const byte Pin_I2C_SDA = 8; // Default
const byte Pin_I2C_SCL = 9; // Default
const byte Pin_SD_MOSI = 12;
const byte Pin_SD_MISO = 13;
const byte Pin_SD_SCK = 14;
const byte Pin_SD_CS = 15;
const byte Pin_Battery = 17;
const byte Pin_IMU_RX = 38;
const byte Pin_IMU_TX = 39;
const byte Pin_LED_B = 40;
const byte Pin_LED_Y = 41;
#else
const gpio_num_t Pin_Button_Wakeup = GPIO_NUM_0;
const byte Pin_Button0 = Pin_Button_Wakeup;
const byte Pin_SwichEN = 2;
const byte Pin_I2C_SDA = 21; // Default
const byte Pin_I2C_SCL = 22; // Default
const byte Pin_SD_MOSI = 12;
const byte Pin_SD_MISO = 13;
const byte Pin_SD_SCK = 14;
const byte Pin_SD_CS = 15;
const byte Pin_Battery = 34;
const byte Pin_IMU_TX = -1;
const byte Pin_IMU_RX = 26;
const byte Pin_LED_B = 25;
const byte Pin_LED_Y = 25;
#endif

#include "SerialDebug.h"
SerialDebug D;
extern SerialDebug Debug = D;

#include <NimBLEDevice.h>
#include "IMU901.h"
#include "OLED.h"
#include "BLE.h"
#include "LongPressSwich.h"
#include "SDCard.h"
#include "RealTimeClock.h"
#include "LEDFlash.h"
#include "Battery.h"

TaskHandle_t *T_FAST;
TaskHandle_t *T_LOOP;
TaskHandle_t *T_SLOW;
TaskHandle_t *T_SEND;
TaskHandle_t *T_SAVE;
TaskHandle_t *T_BUFF;
TaskHandle_t *T_READ;

// RTC_DATA_ATTR int bootCount = 0;

BLE ble;
IMU901 imu;
OLED oled;
LongPressSwich Swich;
SDCard sdCard;
RealTimeClock Clock;
LEDFlash LED;
Battery Bat;
RTC_DATA_ATTR float Save_Roll_Displacement = 0;

String Msg = "";
byte isSDSave = sdCard.SDOK;
int MainLoopDelay = 25;

static void LOOP(void *pvPArameter)
{ // Core 1 Main Function
  for (;;)
  {
    // Check whether long press (Turn Off)
    Swich.Off_Clock_Check();

    // Update IMU
    imu.Update();
    // put I2C device update in same task will be better. (in case library contain delay())
    delay(1);
    Clock.update();
    delay(1);
    oled.Update(imu);
    vTaskDelay(MainLoopDelay);
  }
}

static void Send(void *pvPArameter)
{
  ble.Initialize();
  for (;;)
  {
    ble.Send(imu);
    vTaskDelay(200);
  }
}

static void Save(void *pvParameter)
{
  vTaskDelay(5000);
  sdCard.SetPin(Pin_SD_SCK, Pin_SD_MISO, Pin_SD_MOSI, Pin_SD_CS);
  for (;;)
  {
    isSDSave = sdCard.Save("/" + ble.MyAddress + "/" + Clock.DateStamp("", 2), Msg);
    vTaskDelay(10000);
  }
}

static void FAST(void *pvParameter)
{ // Core 1 secondry function (Fast).
  for (;;)
  {
    bool LED_B[3];
    LED_B[0] = !digitalRead(Pin_Button0);
    LED_B[1] = !ble.isConnect;
    LED_B[2] = ble.isConnect;
    LED.Set(Pin_LED_B, LED_B);
    bool LED_Y[3];
    LED_Y[0] = imu.ErrorCode > (imu.Err_IMU_Cope_Failed + 10);
    LED_Y[1] = (isSDSave != sdCard.SDOK && isSDSave != sdCard.Err_File_Not_Create);
    LED_Y[2] = (Clock.tInvalid != 0);
    LED.Set(Pin_LED_Y, LED_Y);
    vTaskDelay(500);
  }
}

static void SLOW(void *pvParameter)
{ // Core 1 secondry function (Slow).
  for (;;)
  {
    Bat.Update();
    vTaskDelay(1000);
  }
}

static void Buff(void *pvParameter)
{ // Save data in buffer.
  for (;;)
  {
    if (imu.ErrorCode != imu.Err_IMU_Not_Warm_Up && imu.ErrorCode < imu.Err_IMU_Cope_Failed + 10)
    {
      Msg += Clock.DateTimeStamp() + ",";
      Msg += String(imu.Angle[0], 3) + ",";
      Msg += String(imu.Angle[1], 3) + ",";
      Msg += String(imu.Angle[2], 3) + ",";
      Msg += String(imu.A[0], 6) + ",";
      Msg += String(imu.A[1], 6) + ",";
      Msg += String(imu.A[2], 6) + ",";
      Msg += String(imu.G[0], 6) + ",";
      Msg += String(imu.G[1], 6) + ",";
      Msg += String(imu.G[2], 6) + ",";
    }
    // Serial.println("T = " + String(imu4.T,2));
    // Serial.println("A[0] : " + String(imu.Angle[0],3) + " / " +  String(imu4.Angle[0],3));
    // Serial.println("A[1] : " + String(imu.Angle[1],3) + " / " +  String(imu4.Angle[1],3));
    vTaskDelay(2000);
  }
}

void ButtonChange0()
{
  if (digitalRead(Pin_Button0))
  { // Release
    Swich.Off_Clock_Stop();
    if (oled.Page == 2)
      Save_Roll_Displacement -= 0.1;
  }
  else
  { // Press
    Swich.Off_Clock_Start();
  }
}

void ButtonChange1()
{
  oled.Mode_Clock = (oled.Mode_Clock == 0) ? millis() : 0;
}

void ButtonChange2()
{
  if (oled.Page == 1)
    oled.Page = 2;
  else if (oled.Page == 2)
    Save_Roll_Displacement += 0.1;
}

void setup()
{
  Swich.On(Pin_Button_Wakeup, Pin_SwichEN, Pin_LED_B, Pin_LED_Y, Pin_Battery);
  Swich.pSD = &sdCard;
  Wire.begin();
  Wire.setClock(400000);
  Debug.Setup(sdCard);
  Clock.Initialize(MainLoopDelay);
  Debug.SetRTC(&Clock);
  Debug.printOnTop("-------------------------ESP_Start-------------------------");
  Bat.SetPin(Pin_Battery);
  oled.Initialize();
  oled.ConnectShow = &ble.ShowAddress;
  oled.ClockShow = &Clock;
  oled.SDShow = &sdCard.Show;
  oled.SDLastCheck = &sdCard.LastCheck;
  oled.Bat = &Bat.Percent;
  imu.RollDisplacement = &Save_Roll_Displacement;
  imu.Initialize(Pin_IMU_RX, Pin_IMU_TX);
  while (!digitalRead(Pin_Button0))
    delay(1);
  xTaskCreatePinnedToCore(FAST, "Fast", 10000, NULL, 2, T_FAST, 1);
  xTaskCreatePinnedToCore(LOOP, "Loop", 10000, NULL, 1, T_LOOP, 1);
  xTaskCreatePinnedToCore(SLOW, "Slow", 10000, NULL, 1, T_SLOW, 0);
  xTaskCreatePinnedToCore(Send, "Send", 10000, NULL, 3, T_SEND, 0);
  xTaskCreatePinnedToCore(Save, "Save", 10000, NULL, 2, T_SAVE, 0);
  xTaskCreatePinnedToCore(Buff, "Buff", 10000, NULL, 1, T_BUFF, 0);
  delay(100);
  pinMode(Pin_Button1, INPUT);
  pinMode(Pin_Button2, INPUT);
  attachInterrupt(digitalPinToInterrupt(Pin_Button0), ButtonChange0, CHANGE);
  attachInterrupt(digitalPinToInterrupt(Pin_Button1), ButtonChange1, FALLING);
  attachInterrupt(digitalPinToInterrupt(Pin_Button2), ButtonChange2, FALLING);
  Serial.println("Setup Done");
}

void loop()
{
}
