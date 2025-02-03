#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <OLED_I2C.h>

OLED  myOLED(6, 7); // Remember to add the RESET pin if your display requires it...

extern uint8_t SmallFont[];

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"  // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      /*
      for (int i = 0; i < rxValue.length(); i++) {
        Serial.print(rxValue[i]);
      }
      */

      scrollText(rxValue);
      //Serial.println();
    }
  }

  void scrollText(String text, int scrollSpeed = 2) {
    
    char textBuffer[text.length() + 1]; // create a buffer to store the C-style string
    strcpy(textBuffer, text.c_str()); // copy the String to the buffer
    strcat(textBuffer, " ");

    for (int i=128; i>=-(34*6); i-=scrollSpeed)
    {
      myOLED.print(textBuffer, i, 32);
      myOLED.update();
    }

    myOLED.clrScr();
    myOLED.update();
  }
};

void setup() {
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);

  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();

  if(!myOLED.begin(SSD1306_128X64))
  while(1);   // In case the library failed to allocate enough RAM for the display buffer...
  myOLED.setFont(SmallFont);
  scrollText("Waiting a client connection to notify...", 3);
}

void loop() {

  if (deviceConnected) {
    pTxCharacteristic->setValue(&txValue, 1);
    pTxCharacteristic->notify();
    txValue++;
    delay(10);  // bluetooth stack will go into congestion, if too many packets are sent
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                   // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // restart advertising
    scrollText("start advertising", 3);
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}

void scrollText(String text, int scrollSpeed) {
  
  char textBuffer[text.length() + 1]; // create a buffer to store the C-style string
  strcpy(textBuffer, text.c_str()); // copy the String to the buffer
  strcat(textBuffer, " ");

  for (int i=128; i>=-(34*6); i-=scrollSpeed)
  {
    myOLED.print(textBuffer, i, 32);
    myOLED.update();
  }

  myOLED.clrScr();
  myOLED.update();
}

