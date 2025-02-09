#define ADAFRUIT_ASCII96 0

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;

unsigned long lastNotifyTime = 0;
const unsigned long notifyInterval = 100;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
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
      scrollText(rxValue, 7, 2);
    }
  }

  void scrollText(String text, int scrollspeed, int textsize) {
    display.setTextSize(textsize);
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);

    int textWidth = text.length() * 6 * textsize;
    int y = (SCREEN_HEIGHT - (8 * textsize)) / 2;

    for (int x = SCREEN_WIDTH; x > -textWidth; x=x-scrollspeed) {
      display.clearDisplay();
      display.setCursor(x, y);
      display.print(text);
      display.display();
    }

    display.clearDisplay();
    display.display();
  }
};

void setup() {
  Serial.begin(115200);
  Wire.begin(6, 7);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;) ;
  }
  display.cp437(true);
  display.clearDisplay();

  BLEDevice::init("ESP32");

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  pServer->getAdvertising()->start();

  scrollText("Waiting for client", 7, 2);
}

void loop() {

  if (deviceConnected && (millis() - lastNotifyTime >= notifyInterval)) {
    pTxCharacteristic->setValue(&txValue, 1);
    pTxCharacteristic->notify();
    txValue++;
    lastNotifyTime = millis();
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                  
    pServer->startAdvertising(); 
    scrollText("start advertising", 7, 2);
    oldDeviceConnected = deviceConnected;
  }

  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}

void scrollText(String text, int scrollspeed, int textsize) {
  display.setTextSize(textsize);
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);

  int textWidth = text.length() * 6 * textsize;
  int y = (SCREEN_HEIGHT - (8 * textsize)) / 2;

  for (int x = SCREEN_WIDTH; x > -textWidth; x=x-scrollspeed) {
    display.clearDisplay();
    display.setCursor(x, y);
    display.print(text);
    display.display();
  }

  display.clearDisplay();
  display.display();
}

