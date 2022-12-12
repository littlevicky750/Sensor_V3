#include "BLE.h"

void MyServerCallbacks::onConnect(BLEServer *pServer)
{
    *(p->ConnectShow) = "Connecting";
}

void MyServerCallbacks::onDisconnect(BLEServer *pServer)
{
    *(p->isConnect) = false;
    *(p->ConnectShow) = *(p->MyAddress);
    p->NodeNumChar->setValue("");
    BLEDevice::startAdvertising();
}

void NodeNumCallBacks::onWrite(BLECharacteristic *pCharacteristic)
{
    *(p->ConnectShow) = p->NodeNumChar->getValue().c_str();
    *(p->ConnectShow) += '(' + *(p->MyAddress) + ')';
    *(p->isConnect) = true;
}


void BLE::Initialize()
{
    // Start BLE Deviec ----------------------------------------
    BLEDevice::init("ESP32 BLE Children");
    String Address = BLEDevice::getAddress().toString().c_str();
    MyAddress.setCharAt(0, Address.charAt(12));
    MyAddress.setCharAt(1, Address.charAt(13));
    MyAddress.setCharAt(2, Address.charAt(15));
    MyAddress.setCharAt(3, Address.charAt(16));
    MyAddress.toUpperCase();
    ShowAddress = MyAddress;

    // Create Server --------------------------------------------
    BLEServer *pServer = BLEDevice::createServer();

    // Create Address -------------------------------------------
    BLEUUID ServiceUUID("2222eeee-00f2-0123-4567-abcdef123456");
    BLEUUID RollAngUUID("a001eeee-00f2-0123-4567-abcdef123456");
    BLEUUID PitcAngUUID("a002eeee-00f2-0123-4567-abcdef123456");
    BLEUUID YawwAngUUID("a003eeee-00f2-0123-4567-abcdef123456");
    BLEUUID NodeNumUUID("0000eeee-00f2-0123-4567-abcdef123456");

    // Create Service -------------------------------------------
    BLEService *pService = pServer->createService(ServiceUUID);

    // Create Characteristic ------------------------------------
    RollAngChar = pService->createCharacteristic(RollAngUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    PitcAngChar = pService->createCharacteristic(PitcAngUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    YawwAngChar = pService->createCharacteristic(YawwAngUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    NodeNumChar = pService->createCharacteristic(NodeNumUUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE);

    // Set Characteristic Callback ------------------------------
    CBVariable.ConnectShow = &ShowAddress;
    CBVariable.isConnect = &isConnect;
    CBVariable.NodeNumChar = NodeNumChar;
    CBVariable.MyAddress = &MyAddress;

    NodeNumCB.p = &CBVariable;
    NodeNumChar->setCallbacks(&NodeNumCB);

    // Add Descriptor (Avoid using UUID 2902 (Already used by NimBLE))
    // Add 2901 Descriptor
    RollAngChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 25)->setValue("Roll");
    PitcAngChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 25)->setValue("Pitch");
    YawwAngChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 25)->setValue("Yaw");
    NodeNumChar->createDescriptor("2901", NIMBLE_PROPERTY::READ, 30)->setValue("Local ID");

    // Set input type and unit ------------------------------------------
    BLE2904 *p2904Angle = new BLE2904();
    BLE2904 *p2904UTF8s = new BLE2904();

    p2904Angle->setFormat(p2904Angle->FORMAT_FLOAT32); // Format float
    p2904UTF8s->setFormat(p2904UTF8s->FORMAT_UTF8);    // Format String

    p2904Angle->setUnit(0x2763); // plane angle (degree)

    RollAngChar->addDescriptor(p2904Angle);
    PitcAngChar->addDescriptor(p2904Angle);
    YawwAngChar->addDescriptor(p2904Angle);
    NodeNumChar->addDescriptor(p2904UTF8s);

    RollAngChar->setValue(0.0F);
    PitcAngChar->setValue(0.0F);
    YawwAngChar->setValue(0.0F);

    ServerCB.p = &CBVariable;
    pServer->setCallbacks(&ServerCB);
    // Start the Service ---------------------------------------------
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(ServiceUUID);
    pAdvertising->setScanResponse(true);
    BLEDevice::startAdvertising();
}

void BLE::Send(IMU901 &pIMU)
{
    if (pIMU.ErrorCode == pIMU.IMU_Update_Success)
    {
        RollAngChar->setValue(pIMU.Angle[0]);
        PitcAngChar->setValue(pIMU.Angle[1]);
        YawwAngChar->setValue(pIMU.Angle[2]);
        RollAngChar->notify(true);
        PitcAngChar->notify(true);
        YawwAngChar->notify(true);
    }
}