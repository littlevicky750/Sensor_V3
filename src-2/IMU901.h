#ifndef IMU901_h
#define IMU901_h
#include <Arduino.h>
#include <JY901.h>
#include "SerialDebug.h"

#ifndef IMU_C
#define IMU_C 3
#endif

class IMU901
{
private:
    float AngleMeasure[IMU_C][3];
    int IMUStart = 0;
    int CopeFailed = 0;

public:
    float Angle[3] = {0, 0, 0};
    float A[3] = {0, 0, 0};
    float G[3] = {0, 0, 0};
    float *RollDisplacement;
    const byte IMU_Update_Success = 0;
    const byte Err_IMU_Not_Warm_Up = 1;
    const byte Err_IMU_Receive_Data_Error = 2;
    const byte Err_IMU_Data_StDev_Outside_TrustHold = 3;
    const byte Err_IMU_Cope_Failed = 4;
    byte ErrorCode = Err_IMU_Not_Warm_Up;

    void Initialize(byte Rx /*(-1)*/, byte Tx /*(-1)*/)
    {
        Serial1.setRxBufferSize(256);
        Serial1.begin(9600, SERIAL_8N1, Rx, Tx);
        IMUStart = millis();
        memset(AngleMeasure, 0, sizeof(AngleMeasure));
        Debug.println("Recover Displacement from memory = " + String(*RollDisplacement, 2));
    }

    byte Update()
    {
        // read multiple time in case of wrong coping
        for (int j = 0; j < IMU_C * 2; j++)
        {
            // Cope Data from Serial1 --------------------------
            CopeFailed++;
            // Serial.print("Copeing");
            while (Serial1.available())
            {
                // Serial.print(".");
                JY901.CopeSerialData(Serial1.read());
                CopeFailed = 0;
            }
            // Serial.println("");

            float AngleCope[3] = {0, 0, 0};
            float Avg[3] = {0, 0, 0};

            // Check if Cope -------------------------------------
            if (CopeFailed != 0)
            {
                ErrorCode = CopeFailed - 1 + Err_IMU_Cope_Failed;
                goto NextLoop;
            }

            // Read Angle and do basic check -----------------------------
            for (size_t i = 0; i < 3; i++)
            {
                AngleCope[i] = JY901.stcAngle.Angle[i] / 32768.0 * 180.0 ;
                A[i] = JY901.stcAcc.a[i] / 37268.0 * 16.0;
                G[i] = JY901.stcGyro.w[i] / 32768.0 * 2000.0;
                if (AngleCope[i] == 0 || abs(AngleCope[i]) > 180)
                {
                    ErrorCode = Err_IMU_Receive_Data_Error;
                    goto NextLoop;
                }
            }

            // Save Angle------------------------------------------------
            memmove(&AngleMeasure[1][0], &AngleMeasure[0][0], sizeof(AngleMeasure[0][0]) * 3 * (IMU_C - 1));
            memmove(&AngleMeasure[0][0], &AngleCope, sizeof(AngleCope));

            // Filt out noise -----------------------------------
            for (int i = 0; i < IMU_C; i++)
            {
                Avg[0] += AngleMeasure[i][0] / IMU_C;
                Avg[1] += AngleMeasure[i][1] / IMU_C;
                Avg[2] += AngleMeasure[i][2] / IMU_C;
            }
            for (int i = 0; i < 3 * IMU_C; i++)
            {
                if (abs(AngleMeasure[i / 3][i % 3] - Avg[i % 3]) > 30)
                {
                    ErrorCode = Err_IMU_Data_StDev_Outside_TrustHold;
                    goto NextLoop;
                }
            }

            // Output Angle -----------------------------------
            Angle[0] = -AngleMeasure[0][0] + *RollDisplacement;
            Angle[1] = -AngleMeasure[0][1];
            Angle[2] = (AngleMeasure[0][2] > 0) ? (AngleMeasure[0][2] - 180) : (AngleMeasure[0][2] + 180);
            ErrorCode = IMU_Update_Success;
            break;
        NextLoop:
            delay(10);
        } // end for

        // Check Warm Up Time ------------------------------
        if (millis() - IMUStart < 25 * 1000)
        {
            ErrorCode = Err_IMU_Not_Warm_Up;
        }
        return ErrorCode;
    } // end void Update()
};

#endif