#include "OLED.h"
#include "SerialDebug.h"

#ifdef S2
U8G2_SSD1309_128X64_NONAME0_F_HW_I2C u8g2(U8G2_R0);
#else
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0 /*, /* reset=U8X8_PIN_NONE*/);
#endif

void OLED::TestAddress()
{
    if (!isU8G2Begin)
    {
        while (millis() < 2000)
            ;
        Wire.begin();
        Wire.beginTransmission(60);
        byte error = Wire.endTransmission();
        if (error == 0)
            I2C_Add = 0x3C;
        else
        {
            Wire.beginTransmission(61);
            error = Wire.endTransmission();
            if (error == 0)
                I2C_Add = 0x3D;
        }

        if (I2C_Add == 0x00)
        {
            Debug.println("[OLED] OLED Address Not Found. Try to begin u8g2 anyway.");
        }
        else
        {
            u8g2.setI2CAddress(I2C_Add * 2);
            Debug.println("[OLED] OLED begin, Address = 0x" + String(I2C_Add, HEX));
        }
        u8g2.begin();
        u8g2.setFlipMode(0);
        isU8G2Begin = true;
        u8g2.clear();
        u8g2.sendBuffer();
    }
}

void OLED::Initialize()
{
    TestAddress();
    u8g2.enableUTF8Print();
    u8g2.setFont(u8g2_font_8x13B_tf);
    u8g2.setFontDirection(0);
    u8g2.setFontMode(1);
    u8g2.setDrawColor(1);
    u8g2.clearBuffer();
    int w = u8g2.getStrWidth("Wonder Construct");
    u8g2.drawStr(64-w/2, 30, "Wonder Construct");
    w = u8g2.getStrWidth("Sensor V3.0");
    u8g2.drawStr(64-w/2, 50, "Sensor V3.0");
    u8g2.sendBuffer();
}

void OLED::Clear()
{
    TestAddress();
}

void OLED::ShowLowPower()
{
    TestAddress();
    u8g2.setFont(u8g2_font_open_iconic_all_4x_t);
    u8g2.setFontDirection(0);
    u8g2.setFontMode(1);
    u8g2.clearBuffer();
    u8g2.drawRBox(20, 10, 88, 49, 7);
    u8g2.drawRBox(99, 27, 15, 15, 7);
    u8g2.setDrawColor(2);
    u8g2.drawBox(25, 15, 78, 39);
    u8g2.drawGlyph(49, 50, 283);
    u8g2.sendBuffer();
}

void OLED::TurnOff()
{
    TestAddress();
    u8g2.setPowerSave(1);
}

void OLED::Update(IMU901 &imu)
{
    if (millis() < 5000)
    {
        return;
    }
    u8g2.clearBuffer();
    /*
    if (imu.Angle[1] > 40)
    {
        u8g2.setDisplayRotation(U8G2_R3);
        DrawV(imu);
    }
    else if (imu.Angle[1] < -40)
    {
        u8g2.setDisplayRotation(U8G2_R1);
        DrawV(imu);
    }
    else if (imu.Angle[0] < -15)
    {
        u8g2.setDisplayRotation(U8G2_R2);
        DrawH(imu);
    }
    else
    {
        u8g2.setDisplayRotation(U8G2_R0);
        DrawH(imu);
    }
    */
    DrawH(imu);
    u8g2.sendBuffer();
}

void OLED::DrawV(IMU901 &imu)
{
    u8g2.setFont(u8g2_font_8x13B_tf);
    u8g2.drawStr(0, 12, "Test");
}

void OLED::DrawH(IMU901 &imu)
{
    // Battery ---------------------------------------------------------------------
    int BattBox = (int)(*Bat / 100.0 * 20.0 + 0.5);
    BattBox = max(BattBox, 0);
    u8g2.setFont(u8g2_font_siji_t_6x10);
    u8g2.drawGlyph(-2, 62, 57408);
    for (int i = 0; i < BattBox; i++)
    {
        u8g2.drawBox(9 + i * 6, 54, 5, 10);
    }
    for (int i = BattBox; i < 20; i++)
    {
        u8g2.drawFrame(9 + i * 6, 54, 5, 10);
    }

    // Check Page -------------------------------------------------
    if (Page == 1 && millis() - Mode_Clock > 20000)
    {
        Mode_Clock = 0;
    }
    if(Mode_Clock == 0)
        Page = 0;
    else if (Page != 2)
        Page = 1;

    // Frame -----------------------------------------------------
    if (ConnectShow->length() == 8 && Page == 0)
    {
        u8g2.drawFrame(0, 0, 58, 52);
        u8g2.drawFrame(60, 0, 68, 52);
    }
    else
    {
        u8g2.drawFrame(0, 0, 128, 52);
    }

    u8g2.setFont(u8g2_font_8x13B_tf);
    if (Page == 2)
    {
        u8g2.drawStr(4,15,"Expert Mode");
        u8g2.drawStr(8,31,"dtheta:");
        u8g2.drawStr(64,31,String(*imu.RollDisplacement).c_str());
        u8g2.drawStr(8,47,"NowDeg:");
        u8g2.drawStr(64,47,String(imu.Angle[0]).c_str());
    }
    else if (Page == 1) // Page 1 -------------------------------------------------------------------
    {
        u8g2.drawStr(32, 15, ConnectShow->c_str());
        u8g2.drawStr(4, 15, "ID:");
        u8g2.drawFrame(6, 21, 13, 10);
        u8g2.drawHLine(6, 20, 5);
        u8g2.drawHLine(11, 22, 8);
        u8g2.drawStr(20, 31, ":");
        if (SDShow->startsWith("No"))
        {
            int t = millis() - *SDLastCheck;
            String Scanning = (t < 1000)   ? "Scanning."
                              : (t < 2000) ? "Scanning.."
                              : (t < 3000) ? "Scanning..."
                                           : "No Card";
            u8g2.drawStr(32, 31, Scanning.c_str());
        }
        else
        {
            // Count %= (SDShow->length() *3);
            u8g2.drawStr(32, 31, SDShow->c_str());
            // Count++;
        }
        u8g2.drawCircle(12, 41, 5);
        u8g2.drawCircle(12, 41, 6);
        u8g2.drawHLine(12, 41, 3);
        u8g2.drawVLine(12, 38, 3);
        u8g2.drawStr(20, 47, ":");
        u8g2.drawStr(32, 47, ClockShow->TimeStamp().c_str());
    }
    else if(Page == 0) // Page 0 -------------------------------------------------------------------
    {
        // Show Angle --------------------------------------------------------
        if (imu.ErrorCode > imu.Err_IMU_Cope_Failed + 5)
        {
            if (millis() % 2000 < 1000) // "!"
            {
                uint16_t x = (ConnectShow->length() == 8) ? 92 : 64;
                u8g2.drawBox(x, 10, 5, 20);
                u8g2.drawBox(x, 35, 5, 5);
            }
        }
        else if (imu.ErrorCode == imu.Err_IMU_Not_Warm_Up) // "O"
        {
            int x = (ConnectShow->length() == 8) ? 94 : 64;
            int t = Count % 20;
            uint16_t DotSize[4] = {1, 2, 2, 3};
            for (int i = 0; i < 4; i++)
            {
                u8g2.drawDisc(x + 10 * sin(t / 20.0 * 2.0 * PI), 26 - 10 * cos(t / 20.0 * 2.0 * PI), DotSize[i]);
                t = (t + 3) % 20;
            }
            Count++;
        }
        else
        {
            char AngleShow[8];
            for (uint16_t i = 0; i < 3; i++)
            {
                dtostrf(imu.Angle[i], 7, 2, AngleShow);
                u8g2.drawStr(68, 15 + 16 * i, AngleShow);
            }
        }

        // Other Show
        if (ConnectShow->length() == 8)
        {
            u8g2.setFont(u8g2_font_inb30_mr);
            u8g2.drawGlyph(6, 40, ConnectShow->charAt(0));
            u8g2.drawGlyph(30, 40, ConnectShow->charAt(1));
        }
        else
        {
            if (imu.ErrorCode != imu.Err_IMU_Not_Warm_Up && imu.ErrorCode < imu.Err_IMU_Cope_Failed + 5)
            {
                u8g2.drawStr(4, 15, "Roll:");
                u8g2.drawStr(4, 31, "Pitch:");
                u8g2.drawStr(4, 47, "Yaw:");
            }
        }
    }
}
