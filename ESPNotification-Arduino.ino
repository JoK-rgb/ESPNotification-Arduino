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

#define BEEPER_PIN  14

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
      displayText(rxValue, 1, 5000);
    }
  }

  String* splitString(String inputString) {
    static String parts[3];
    int firstSep = inputString.indexOf("||");
    int secondSep = inputString.indexOf("||", firstSep + 2);
    if (firstSep != -1 && secondSep != -1) {
      parts[0] = inputString.substring(0, firstSep);
      parts[1] = inputString.substring(firstSep + 2, secondSep);
      parts[2] = inputString.substring(secondSep + 2);
    } else {
      parts[0] = parts[1] = parts[2] = "";
    }
    return parts;
  }

  String wordWrap(String text, int maxWidth) {
    String result = "";
    String currentLine = "";
    int16_t x1, y1;
    uint16_t w, h;

    int start = 0;
    int spaceIndex = text.indexOf(' ', start);
    while (spaceIndex != -1) {
      String word = text.substring(start, spaceIndex);
      if (currentLine.length() == 0) {
        display.getTextBounds(word.c_str(), 0, 0, &x1, &y1, &w, &h);
        if (w > maxWidth) {
          for (int i = 0; i < word.length(); i++) {
            String testWord = currentLine + word.charAt(i);
            display.getTextBounds(testWord.c_str(), 0, 0, &x1, &y1, &w, &h);
            if (w > maxWidth) {
              result += currentLine + "\n";
              currentLine = "";
              currentLine += word.charAt(i);
            } else {
              currentLine += word.charAt(i);
            }
          }
        } else {
          currentLine = word;
        }
      }
      else {
        String testLine = currentLine + " " + word;
        display.getTextBounds(testLine.c_str(), 0, 0, &x1, &y1, &w, &h);
        if (w > maxWidth) {
          result += currentLine + "\n";
          currentLine = word;
        } else {
          currentLine = testLine;
        }
      }
      start = spaceIndex + 1;
      spaceIndex = text.indexOf(' ', start);
    }
    String remaining = text.substring(start);
    if (remaining.length() > 0) {
      if (currentLine.length() > 0) {
        String testLine = currentLine + " " + remaining;
        display.getTextBounds(testLine.c_str(), 0, 0, &x1, &y1, &w, &h);
        if (w > maxWidth) {
          result += currentLine + "\n" + remaining;
        } else {
          result += testLine;
        }
      } else {
        result += remaining;
      }
    } else {
      result += currentLine;
    }
    return result;
  }

  void displayText(String text, int textsize, int displayTime) {
    tone(BEEPER_PIN, 2000, 100);

    String* stringParts = splitString(text);

    if(stringParts[0] == "" && stringParts[2] != "") {
      // if splitParts[0] is empty it is a information message from the NotificationListener App itself

      scrollText(stringParts[2], 5, 2);
    } else {
      // actual notification

      blockText(stringParts, textsize, displayTime);
    }
  }

  void blockText(String* stringParts, int textsize, int displayTime) {
      display.clearDisplay();
      display.setTextSize(textsize);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.setTextWrap(false);

      display.println(stringParts[0]);
      display.println(stringParts[1]);

      int lineY = display.getCursorY();
      display.drawLine(0, lineY, display.width() - 1, lineY, SSD1306_WHITE);
      display.println();

      String wrappedThird = wordWrap(stringParts[2], display.width());
      display.println(wrappedThird);

      display.display();
      delay(displayTime);

      display.clearDisplay();
      display.display();
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

  BLEDevice::init("ESP32 Notification Display");

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  pServer->getAdvertising()->start();

  scrollText("Waiting for client", 5, 2);
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
    scrollText("start advertising", 5, 2);
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


