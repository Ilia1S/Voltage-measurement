#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define bleServerName "ESP32_Ilia"
#define ADCP 34
#define SERVICE_UUID "9ec27367-2c5a-40b8-9183-bc6376ad093f"
#define N 10 //number of iterations for SMA calculation
#define MA 2 //SMA -1, EMA - 2
#define E 0.1 // EMA coefficient

BLECharacteristic bmeBatteryLevelCharacteristics("d0a3ae62-7279-47e6-a9c3-c39ab277718c", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor bmeBatteryLevelDescriptor(BLEUUID((uint16_t)0x27AD));

float batVol = 0, batLev = 0, MAb = 0, MAv = 0;
bool deviceConnected = false;
uint16_t timerDelay = 5000;
uint32_t lastTime = 0;

class MyServerCallbacks: public BLEServerCallbacks 
{
  void onConnect(BLEServer* pServer) 
  {
    deviceConnected = true;
  };
  void onDisconnect(BLEServer* pServer) 
  {
    deviceConnected = false;
  }
};

void setup() 
{
  Serial.begin(115200);
  pinMode(ADCP,INPUT_PULLUP);
  BLEDevice::init(bleServerName);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *bmeService = pServer->createService(SERVICE_UUID);
  bmeService->addCharacteristic(&bmeBatteryLevelCharacteristics);
  bmeBatteryLevelDescriptor.setValue("Battery Level");
  bmeBatteryLevelCharacteristics.addDescriptor(new BLE2902());
  bmeService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
  #if MA==1
    Serial.println("SMA method is selected...");
  #elif MA==2
    Serial.println("EMA method is selected...");
  #endif
}
void loop() 
{
  if (deviceConnected)
  {
    if ((millis() - lastTime) > timerDelay)
    {
      #if MA==1
        for (uint8_t i=0; i<N; i++)
        {
          batVol = (analogRead(ADCP)*3.3)/4096;
          if (batVol > 0.9)
            batLev = ((batVol - 0.9)/0.7)*100;
          else
            batLev = 0;
          MAv = MAv + batVol/N;
          MAb = MAb + batLev/N;
          delay(20);
        }
      #elif MA==2
        for (uint8_t i=0; i<N; i++)
        {
          if (i==0)
          {
            MAv = (analogRead(ADCP)*3.3)/4096;
            if (MAv > 0.9)
              MAb = ((batVol - 0.9)/0.7)*100;
            else
              MAb = 0;
          }
          else
          {
            batVol = (analogRead(ADCP)*3.3)/4096;
            if (batVol > 0.9)
              batLev = ((batVol - 0.9)/0.7)*100;
            else
              batLev = 0;
            MAv = E*batVol + (1-E)*MAv;
            MAb = E*batLev + (1-E)*MAb;
          }
          delay(20);
        }
      #endif
      Serial.print(" - Voltage ");
      Serial.print(MAv);
      Serial.println(" V");
      Serial.print(" - Battery Level ");
      Serial.print(MAb);
      Serial.println(" %");
      static char MAChar[6];
      dtostrf(MAb, 5, 2, MAChar);
      bmeBatteryLevelCharacteristics.setValue(MAChar);
      bmeBatteryLevelCharacteristics.notify();
      MAv = 0, MAb = 0;
      lastTime = millis();
    }
  }
}



 /*     #elif METHOD==SMAR
      for (uint8_t i=0; i<N; i++)
      {
        if (i==0)
        {
          v1 = (analogRead(ADCP)*3.3)/(4096*N);
          if (a > 0.9)
            b1 = ((v1 - 0.9)/0.7)*100;
          else
            b1 = 0;
        }
        if (i>=0 && N-2>=i)
        {
          batVol = (analogRead(ADCP)*3.3)/4096;
          if (batVol > 0.9)
            batLev = ((batVol - 0.9)/0.7)*100;
          else
            batLev = 0;
          SMAv = SMAv + batVol/N;
          SMAb = SMAb + batLev/N;
        }
        if (i==N-1)
        {
          v2 = (analogRead(ADCP)*3.3)/(4096*N);
          if (batVol > 0.9)
            b2 = ((batVol - 0.9)/0.7)*100;
          else
            b2 = 0;
          SMAv = SMAv - v1 + v2;
          SMAb = SMAb - b1 + b2;
        }
        delay(20);
      } */ /*     #elif METHOD==SMAR
      for (uint8_t i=0; i<N; i++)
      {
        if (i==0)
        {
          v1 = (analogRead(ADCP)*3.3)/(4096*N);
          if (a > 0.9)
            b1 = ((v1 - 0.9)/0.7)*100;
          else
            b1 = 0;
        }
        if (i>=0 && N-2>=i)
        {
          batVol = (analogRead(ADCP)*3.3)/4096;
          if (batVol > 0.9)
            batLev = ((batVol - 0.9)/0.7)*100;
          else
            batLev = 0;
          SMAv = SMAv + batVol/N;
          SMAb = SMAb + batLev/N;
        }
        if (i==N-1)
        {
          v2 = (analogRead(ADCP)*3.3)/(4096*N);
          if (batVol > 0.9)
            b2 = ((batVol - 0.9)/0.7)*100;
          else
            b2 = 0;
          SMAv = SMAv - v1 + v2;
          SMAb = SMAb - b1 + b2;
        }
        delay(20);
      } */
