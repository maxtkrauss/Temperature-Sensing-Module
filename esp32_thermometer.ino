#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <ThingSpeak.h>
#include <EEPROM.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <NimBLE2904.h>

const char* ssid = "Guest";
String password = "";

// Allocate adequate space for the EEPROM and init pins
#define EEPROM_SIZE 40
#define BUTTON_PIN 13
#define SENSOR_PIN 23
#define RED_LED 25
#define ORANGE_LED 26
#define BLUE_LED 27

// Init temp sensing modules
OneWire oneWire(SENSOR_PIN);
DallasTemperature DS18B20(&oneWire);

// Init wifi client for Thingspeak communication
WiFiClient client;

// Init timing variables
const long intervalUpload = 900000;
unsigned long previousMillisUpload = -intervalUpload;

// Init Thingspeak variables to write temp data
// Channel 1
const char* channelIDStr_write = "2293055";
const char* apiKey_write = "QMPHX07FG72F2PMD";

// Channel 2
// const char* channelIDStr_write = "1694548";
// const char* apiKey_write = "1RJWH4UGTYE9HONA";

// Field:
int field = 4;

const char* server = "api.thingspeak.com";
unsigned long channelID_write = atol(channelIDStr_write);

// Init Thingspeak variables to read password updates
const char* channelIDStr_read = "2277799";
unsigned long channelID_read = atol(channelIDStr_read);
const char* apiKey_read = "8L2M0OE8TLJUEAX5";
const char* apiKey_read_w = "Z99AJIYCC4VLRL4K";

// Define a custom service and characteristic Universally Unique ID for BLE
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" 
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// Init BLE server variables
NimBLEServer* pServer = NULL;
NimBLECharacteristic* pCharacteristic = NULL;
bool bleConnected = false;

// Init credential sorting variables
String newCred;
String temp;
String thingspeakCredsOnBoot;
String strs[2];
int StringCount = 0;
String EEPROMcreds;

// Init remaining data/state indicator variables
float tempF;
bool button_pressed = false;
bool waiting = false;

// Callbacks for BLE server connections/disconnections
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
      bleConnected = true;
      Serial.println("BLE Client Connected");
    };
    void onDisconnect(NimBLEServer* pServer) {
      bleConnected = false;
      Serial.println("BLE Client Disconnected");
    }
};

// Callbacks for when BLE Server receives new credentials
class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
      // Handle write events for the BLE characteristic
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        temp += String(value.c_str());
        Serial.println(temp);
      }
    }
};

// LED State Indication Functions 
void red() {
  digitalWrite(RED_LED, HIGH); digitalWrite(ORANGE_LED, LOW); digitalWrite(BLUE_LED, LOW);
}
void orange() {
  digitalWrite(RED_LED, LOW); digitalWrite(ORANGE_LED, HIGH); digitalWrite(BLUE_LED, LOW);
}
void blue() {
  digitalWrite(RED_LED, LOW); digitalWrite(ORANGE_LED, LOW); digitalWrite(BLUE_LED, HIGH);
}
void cyan() {
  digitalWrite(RED_LED, LOW); digitalWrite(ORANGE_LED, HIGH); digitalWrite(BLUE_LED, HIGH);
}
void yellow() {
  digitalWrite(RED_LED, HIGH); digitalWrite(ORANGE_LED, HIGH); digitalWrite(BLUE_LED, LOW);
}
void purple() {
  digitalWrite(RED_LED, HIGH); digitalWrite(ORANGE_LED, LOW); digitalWrite(BLUE_LED, HIGH);
}
void off() {
  digitalWrite(RED_LED, LOW); digitalWrite(ORANGE_LED, LOW); digitalWrite(BLUE_LED, LOW);
}
void rainbowWipe() {
  red(); delay(150); orange(); delay(150); blue(); delay(150); red(); delay(150);
  orange(); delay(150); blue(); delay(150); off();
}
void errorFlash() {
  red(); delay(200); off(); delay(100); red(); delay(200); off(); delay(100);
  red(); delay(200); off(); delay(100); red(); delay(200); off(); delay(1000);
}
void bluetoothflash() {
  blue(); delay(150); blue(); delay(150); blue(); delay(150); blue(); delay(150);
  blue(); delay(150); blue(); delay(150); blue();
}
void wifiDelay() {
  yellow(); delay(1000); off(); delay(1000); bluetooth();
  yellow(); delay(1000); off(); delay(1000); bluetooth();
}

void bluetooth() {
  // Check if the button is pressed to initiate BLE communication
  if (digitalRead(BUTTON_PIN) == HIGH) {
    bluetoothflash();
    // Initialize BLE server and characteristics
    NimBLEDevice::init("ESP32 BLE Test");
    NimBLEServer* pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    NimBLEService* pService = pServer->createService(SERVICE_UUID);
    // RX Characteristic (Write)
    NimBLECharacteristic* pRXCharacteristic = pService->createCharacteristic(
          CHARACTERISTIC_UUID_RX,
          NIMBLE_PROPERTY::WRITE
        );
    pRXCharacteristic->setCallbacks(new MyCallbacks());
    // TX Characteristic (Notify)
    pCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
                      );
    pCharacteristic->addDescriptor(new NimBLE2904());
    pService->start();
    pServer->getAdvertising()->start();
    Serial.println("Waiting a client connection to notify...");
    waiting = true;
    while (waiting) {
      if (temp != "") {
        newCred = temp;
        temp = "";
        waiting = false;
      }
    }
    Serial.println("New Credentials Received!");
    Serial.println(newCred);
    EEPROM.writeString(0, newCred);
    EEPROM.commit();
    blue(); delay(150); off(); delay(100); blue(); delay(150); off(); delay(100);
    blue(); delay(150); off(); delay(100); blue(); delay(150); off(); delay(100);
    ESP.restart();
  }
}

void setup() {
  pinMode(RED_LED, OUTPUT);
  pinMode(ORANGE_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  rainbowWipe();

  Serial.begin(115200);
  DS18B20.begin();
  EEPROM.begin(EEPROM_SIZE);
  EEPROMcreds = EEPROM.readString(0);
  String creds = EEPROMcreds;
  Serial.println(EEPROMcreds);

  // Split the EEPROMstring into substrings
  while (creds.length() > 0)
  {
    int index = creds.indexOf(',');
    if (index == -1) // No comma found
    {
      strs[StringCount++] = creds;
      break;
    }
    else
    {
      strs[StringCount++] = creds.substring(0, index);
      creds = creds.substring(index + 1);
    }
  }

  Serial.print("SSID:");
  Serial.println(strs[0]);
  Serial.print("Password:" );
  Serial.println(strs[1]);

  WiFi.begin(strs[0], strs[1]);
  wifiDelay();
  wifiDelay();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi with EEPROM Credentials.");
    orange();
  }
  else{
    WiFi.begin(ssid, password);
    Serial.println("Failed to connect through saved credentials.");
    Serial.println("Attempting to connect through hardcoded wifi:");
    Serial.print("Hardcoded SSID:");
    Serial.println(ssid);
    Serial.print("Hardcoded Password:");
    Serial.println(password);
    wifiDelay();
  }

  if(WiFi.status() != WL_CONNECTED){
    red();
  }

  ThingSpeak.begin(client);
  thingspeakCredsOnBoot = ThingSpeak.readFloatField(channelID_read, 1, apiKey_read);
  if (thingspeakCredsOnBoot != EEPROMcreds && WiFi.status() == WL_CONNECTED){
      Serial.println("Connected to WiFi. Pushing credentials to ThingSpeak.");
      Serial.println(EEPROMcreds);
      ThingSpeak.writeField(channelID_read, 1, EEPROMcreds, apiKey_read_w);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {

  bluetooth();
  unsigned long currentMillis = millis();

  // Read temperature and send data to Thingspeak every 15 minutes
  if (WiFi.status() == WL_CONNECTED) {
      if (currentMillis - previousMillisUpload >= intervalUpload) {
          // Perform temperature reading
          DS18B20.requestTemperatures();
          tempF = ((DS18B20.getTempCByIndex(0)) * 1.8) + 32;
          Serial.println(tempF);

          //Correct temperature reading from calibration


          // Send data to ThingSpeak field
          ThingSpeak.setField(field, tempF);
          int response = ThingSpeak.writeFields(channelID_write, apiKey_write);
          // Check response and handle accordingly
          if (response == 200) {
            Serial.println("Data sent to ThingSpeak successfully.");
            previousMillisUpload = currentMillis;
            cyan();
            delay(2500); bluetooth(); delay(2500);
          } else {
            Serial.println("Error sending data to ThingSpeak. HTTP error code: " + String(response));
            purple();
            delay(2500); bluetooth(); delay(2500);
          }
        }
      else{
        Serial.println("Waiting for next upload.");
        yellow();
        delay(2500); bluetooth(); delay(2500);
      }
    // Code for reading password updates from Thingspeak
    newCred = ThingSpeak.readStringField(channelID_read, 1, apiKey_read); // potential newCred from thingspeak
    Serial.println(newCred);
    if (newCred != EEPROMcreds && newCred != "") { 
     Serial.println("New ThingSpeak Credentials Received!");
     Serial.println(newCred);
     EEPROM.writeString(0, newCred);
     EEPROM.commit();

     cyan(); delay(150); red(); delay(100); cyan(); delay(150); red(); delay(100);
     cyan(); delay(150); red(); delay(100); cyan(); delay(150); red(); delay(100);

     ESP.restart();
   }
  }
  else {
    Serial.println("Failed to connect to WiFi. Utilize Bluetooth.");
    bluetooth();
    red();
  }
}