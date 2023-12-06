#include <Arduino.h>

#include <Wire.h>
#include "SparkFun_VCNL4040_Arduino_Library.h"

#include <BLEDevice.h>
#include <BLEServer.h>

#include <WiFi.h>
#include <HTTPClient.h>

// Add #define variables for WiFi
#define SERVICE_UUID "3bb82dd9-afb3-43d3-9c76-f11f090a0cd1"
#define CHARACTERISTIC_UUID "e02ea3fb-6475-4194-b337-583245497450"
#define LED_12 12 // for sensor testing only
#define LED_13 13 // for BLE testing only

VCNL4040 proximitySensor;

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

  Wire.begin();
  if (proximitySensor.begin() == false)
  {
    Serial.println("Device not found. Please check wiring.");
    while (1);
  }

  BLEDevice::init("Security Mailbox");
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new MyServerCallbacks());
  BLEService *service = server->createService(SERVICE_UUID);
  service->start();
  BLEAdvertising *advertising = server->getAdvertising();
  advertising->start();
  Serial.println("BLE setup complete");

  delay(1000);

  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

void loop() {
  unsigned int proxValue = proximitySensor.getProximity();
  Serial.println(proxValue);
  if (proxValue > 100) {
    digitalWrite(LED_12, HIGH);

    WiFiClient client;
    HTTPClient http;
    http.begin(client, MAIL_ENDPOINT);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(MAIL_REQUEST_BODY);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.printf("Error occurred while sending HTTP POST");
    }
    delay(1000);
  } else {
    digitalWrite(LED_12, LOW);
  }

  if (deviceConnected) {
    digitalWrite(LED_13, HIGH);
  } else {
    digitalWrite(LED_13, LOW);
  }
}