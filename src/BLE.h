#ifndef BLE_H
#define BLE_H
#include "NimBLEDevice.h"
#include <Arduino.h>
#include "IMU901.h"

class CBpVariable
{
public:
    BLECharacteristic *NodeNumChar;
    bool *isConnect;
    String *ConnectShow;
    String *MyAddress;
};

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer);
    void onDisconnect(BLEServer *pServer);

public:
    CBpVariable *p;
};

class NodeNumCallBacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic);

public:
    CBpVariable *p;
};

class BLE
{
public:
    bool isConnect = false;
    void Initialize();
    void Send(IMU901 &IMU);
    String MyAddress = "XXXX";
    String ShowAddress;

private:
    BLECharacteristic *RollAngChar;
    BLECharacteristic *PitcAngChar;
    BLECharacteristic *YawwAngChar;
    BLECharacteristic *NodeNumChar;

    CBpVariable CBVariable;
    MyServerCallbacks ServerCB;
    NodeNumCallBacks NodeNumCB;
};
#endif