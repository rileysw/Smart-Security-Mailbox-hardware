#include <Arduino.h>

// Flash memory
#include <Preferences.h>

// Sensor and actuator libaries
#include <ESP32Servo.h>
#include "SparkFun_VCNL4040_Arduino_Library.h"

// Protocol libraries
#include <BLEDevice.h>
#include <BLEServer.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>

// Defined constants
#define LED_12 12
#define SERVO_15 15
#define SDA_2 32
#define SCL_2 33
#define I2C_freq 100000
#define MAIL_ENDPOINT "https://y96ey5lzna.execute-api.us-west-1.amazonaws.com/dev/mail"
#define SERVICE_UUID "00000000-0000-0000-0000-00000000000a"
#define SSID_CHARACTERISTIC_UUID "10000000-0000-0000-0000-00000000000a"
#define PASSWORD_CHARACTERISTIC_UUID "10000000-0000-0000-0000-00000000000b"
#define UID_CHARACTERISTIC_UUID "10000000-0000-0000-0000-00000000000c"

// Global declarations
Preferences preferences;
Servo servoLock;
VCNL4040 timeProximitySensor;
VCNL4040 detectionProximitySensor;
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
  servoLock.attach(SERVO_15);

  Wire.begin();
  if (timeProximitySensor.begin() == false)
  {
    Serial.println("Time of mail sensor not found. Please check wiring.");
    bool ledOn = false;
    while (1) {
      if (ledOn) {
        digitalWrite(LED_12, LOW);
        ledOn = false;
      } else {
        digitalWrite(LED_12, HIGH);
        ledOn = true;
      }
      delay(100);
    }
  }

  Wire1.begin(SDA_2, SCL_2, I2C_freq);
  if (detectionProximitySensor.begin(Wire1) == false) {
    Serial.println("Detection of mail sensor not found. Please check wiring.");
    bool ledOn = false;
    while (1) {
      if (ledOn) {
        digitalWrite(LED_12, LOW);
        ledOn = false;
      } else {
        digitalWrite(LED_12, HIGH);
        ledOn = true;
      }
      delay(100);
    }
  }

  // BLE Characteristics
  BLECharacteristic *ssidCharacteristic = new BLECharacteristic(SSID_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
  BLECharacteristic *passwordCharacteristic = new BLECharacteristic(PASSWORD_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
  BLECharacteristic *uidCharacteristic = new BLECharacteristic(UID_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);

  BLEDevice::init("Security Mailbox");
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new MyServerCallbacks());
  BLEService *service = server->createService(SERVICE_UUID);
  service->addCharacteristic(ssidCharacteristic);
  service->addCharacteristic(passwordCharacteristic);
  service->addCharacteristic(uidCharacteristic);
  service->start();
  BLEAdvertising *advertising = server->getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->start();
  Serial.println("BLE setup complete");

  delay(3000);

  preferences.begin("credentials", false);

  // To test setup
  // preferences.clear();

  if (!preferences.getBool("setup")) {
    Serial.println("Configuring device");

    bool foundSSID = false;
    bool foundPassword = false;
    bool foundUID = false;
    while (!foundSSID || !foundPassword || !foundUID) {
      if (!foundSSID) {
        String ssid = service->getCharacteristic(SSID_CHARACTERISTIC_UUID)->getValue().c_str();
        if (ssid != "") {
          preferences.putString("ssid", ssid);
          Serial.println(preferences.getString("ssid"));
          foundSSID = true;
        }
      }
      if (!foundPassword) {
        String password = service->getCharacteristic(PASSWORD_CHARACTERISTIC_UUID)->getValue().c_str();
        if (password != "") {
          preferences.putString("password", password);
          Serial.println(preferences.getString("password"));
          foundPassword = true;
        }
      }
      if (!foundUID) {
        String uid = service->getCharacteristic(UID_CHARACTERISTIC_UUID)->getValue().c_str();
        if (uid != "") {
          preferences.putString("uid", uid);
          Serial.println(preferences.getString("uid"));
          foundUID = true;
        }
      }
      delay(500);
    }

    preferences.putBool("setup", true);
  }
  Serial.println("Device configured");

  Serial.print("Connecting to ");
  Serial.println(preferences.getString("ssid"));
  WiFi.begin(preferences.getString("ssid"), preferences.getString("password"));
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

void loop() {
  unsigned int timeSensorValue = timeProximitySensor.getProximity();
  if (timeSensorValue > 100) { // update threshold according to physical mailbox design
    WiFiClientSecure *client = new WiFiClientSecure;
    if (client) {
      client->setInsecure();
    }

    HTTPClient http;
    http.begin(*client, MAIL_ENDPOINT);
    http.addHeader("Content-Type", "application/json");

    String responseBody = "{\"uid\": " + preferences.getString("uid") + "}";
    int httpResponseCode = http.POST(responseBody);
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.printf("Error occurred while sending HTTP POST");
    }
    delay(1000);
  }

  unsigned int detectionSensorValue = detectionProximitySensor.getProximity();
  if (detectionSensorValue > 1000) {
    Serial.println("Mail detected");
    // Send detection data to backend
  }

  if (deviceConnected) {
    servoLock.write(90);
  } else {
    servoLock.write(0);
  }
}