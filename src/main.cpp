#include <Arduino.h>

#include <Adafruit_CAP1188.h>

#include <BLEDevice.h>
#include <BLEServer.h>

#define SERVICE_UUID "3bb82dd9-afb3-43d3-9c76-f11f090a0cd1"
#define CHARACTERISTIC_UUID "e02ea3fb-6475-4194-b337-583245497450"
#define LED_12 12 // for sensor testing only
#define LED_13 13 // for BLE testing only

Adafruit_CAP1188 cap = Adafruit_CAP1188();

bool deviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* server) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* server) {
      deviceConnected = false;
      server->getAdvertising()->start();
    }
};

void setup() {
  Serial.begin(9600);

  pinMode(LED_12, OUTPUT);
  pinMode(LED_13, OUTPUT);

  if (!cap.begin())
  {
    Serial.println("CAP1188 not found");
    while (1)
      ;
  }
  Serial.println("CAP1188 found");

  BLEDevice::init("Security Mailbox");
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new MyServerCallbacks());
  BLEService *service = server->createService(SERVICE_UUID);
  service->start();
  BLEAdvertising *advertising = server->getAdvertising();
  advertising->start();
  Serial.println("BLE setup complete");
}

void loop() {
  uint8_t touched = cap.touched();
  if (touched) {
    digitalWrite(LED_12, HIGH);
  } else {
    digitalWrite(LED_12, LOW);
  }

  if (deviceConnected) {
    digitalWrite(LED_13, HIGH);
  } else {
    digitalWrite(LED_13, LOW);
  }
}