#include <iostream>
#include <string>
using namespace std;
#include <WiFi.h>
#include <ESP32httpUpdate.h>
#include <EEPROM.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <Adafruit_NeoPixel.h>

#include <PubSubClient.h>

#include "ota.h"
#include "ble_callbacks.h"
#include "led.h"

#ifndef DEFINITIONS
#include "definitions.h"
#endif

#ifndef PIXEL
#include <Adafruit_NeoPixel.h>
#endif

configuration_struct configuration;
Adafruit_NeoPixel pixels(NUMBER_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
Led led(&pixels);
WiFiClient espClient;
PubSubClient client(espClient);

string real_time_color = "#0000ff";
int number = 0;
long int last_alive = 0;

char alive_topic[50];
char color_topic[50];

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  for (int i=0;i<length;i++) {
    real_time_color[i] == (char)payload[i];
  }
}

void setup() {
  Serial.begin(baudrate);
  // Read the eeprom
  Serial.println("\r\n\r\nWELCOME 2 LED LINK");
  Serial.println("\r\n");
  led.blink(1,1000,1000,0,255,0);
  EEPROM.begin(eeprom_size);  
  EEPROM.get(0, configuration);
  // Connect to WiFi
  Serial.print("Connecting to ");
  Serial.println(configuration.ssid);
  WiFi.begin(configuration.ssid, configuration.password);
  int start_ts = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start_ts < 5000){
    Serial.print(".");
    led.blink(1,500,500,0,0,255);
    //delay(1000);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\r\nConnected");
  } else {
    Serial.println("\r\nFail");
  }

  if (!configuration.updated) {
    Serial.print("updating");    
    configuration.updated = true;
    EEPROM.put(0,configuration);
    EEPROM.commit();
    updateFirmware(&configuration.ota_server[0]);
    Serial.print("updated");
  }
  
  // Set up BLE
  char ble_name[150] = "";
  strcat(ble_name,configuration.ble_name);
  strcat(ble_name," ");
  strcat(ble_name,configuration.color);
  BLEDevice::init(ble_name);
  BLEServer *ble_server = BLEDevice::createServer();

  // Generic Access service
  BLEService *ble_access_service = ble_server->createService("1800");
  // Name characteristic
  BLECharacteristic *name_characteristic = ble_access_service->createCharacteristic(
                                                                   "2A00",
                                                                   BLECharacteristic::PROPERTY_READ
                                                                 );
  name_characteristic->setValue(configuration.ble_name);
  // Device info service
  BLEService *ble_device_info_service = ble_server->createService("180A");
  // Firmware version characteristic
  BLECharacteristic *firmware_version_characteristic = ble_device_info_service->createCharacteristic(
                                                                   "2A28",
                                                                   BLECharacteristic::PROPERTY_READ
                                                                 );
  firmware_version_characteristic->setValue(version_code);
  // Color characteristic service
  BLEService *ble_color_service = ble_server->createService(COLOR_SERVICE_UUID);
  BLECharacteristic *color_characteristic = ble_color_service->createCharacteristic(
                                                                   REAL_TIME_COLOR_UUID,
                                                                   BLECharacteristic::PROPERTY_READ |
                                                                   BLECharacteristic::PROPERTY_WRITE
                                                                 );
  color_characteristic->setValue(real_time_color);
  // Color characteristic
  BLECharacteristic *color_server_characteristic = ble_color_service->createCharacteristic(
                                                                   COLOR_UUID,
                                                                   BLECharacteristic::PROPERTY_READ |
                                                                   BLECharacteristic::PROPERTY_WRITE
                                                                 );
  color_server_characteristic->setCallbacks(new MyCallbacks());
  color_server_characteristic->setValue(configuration.color);
  // Config service
  BLEService *ble_config_service = ble_server->createService(CONFIGURATION_UUID);
  // SSID characteristic
  BLECharacteristic *ssid_characteristic = ble_config_service->createCharacteristic(
                                                                   SSID_UUID,
                                                                   BLECharacteristic::PROPERTY_READ |
                                                                   BLECharacteristic::PROPERTY_WRITE
                                                                 );
  ssid_characteristic->setCallbacks(new MyCallbacks());
  ssid_characteristic->setValue(configuration.ssid);
  // Password characteristic
  BLECharacteristic *password_characteristic = ble_config_service->createCharacteristic(
                                                                   PASSWORD_UUID,
                                                                   BLECharacteristic::PROPERTY_WRITE
                                                                 );
  password_characteristic->setCallbacks(new MyCallbacks());
  // OTA server characteristic
  BLECharacteristic *ota_server_characteristic = ble_config_service->createCharacteristic(
                                                                   OTA_SERVER_UUID,
                                                                   BLECharacteristic::PROPERTY_READ |
                                                                   BLECharacteristic::PROPERTY_WRITE
                                                                 );
  ota_server_characteristic->setCallbacks(new MyCallbacks());
  ota_server_characteristic->setValue(configuration.ota_server);
  // Broker server characteristic
  BLECharacteristic *broker_server_characteristic = ble_config_service->createCharacteristic(
                                                                   BROKER_SERVER_UUID,
                                                                   BLECharacteristic::PROPERTY_READ |
                                                                   BLECharacteristic::PROPERTY_WRITE
                                                                 );
  broker_server_characteristic->setCallbacks(new MyCallbacks());
  broker_server_characteristic->setValue(configuration.broker_server);
  // Topic characteristic
  BLECharacteristic *topic_characteristic = ble_config_service->createCharacteristic(
                                                                   TOPIC_UUID,
                                                                   BLECharacteristic::PROPERTY_READ |
                                                                   BLECharacteristic::PROPERTY_WRITE
                                                                 );
  topic_characteristic->setCallbacks(new MyCallbacks());
  topic_characteristic->setValue(configuration.broker_topic);
  // User characteristic
  BLECharacteristic *broker_user_characteristic = ble_config_service->createCharacteristic(
                                                                   BROKER_USER_UUID,
                                                                   BLECharacteristic::PROPERTY_READ |
                                                                   BLECharacteristic::PROPERTY_WRITE
                                                                 );
  broker_user_characteristic->setCallbacks(new MyCallbacks());
  broker_user_characteristic->setValue(configuration.broker_user);
  // Broker pass characteristic
  BLECharacteristic *broker_pass_characteristic = ble_config_service->createCharacteristic(
                                                                   BROKER_PASS_UUID,
                                                                   BLECharacteristic::PROPERTY_READ |
                                                                   BLECharacteristic::PROPERTY_WRITE
                                                                 );
  broker_pass_characteristic->setCallbacks(new MyCallbacks());
  broker_pass_characteristic->setValue(configuration.broker_pass);
  // Exec service
  BLEService *ble_exec_service = ble_server->createService(EXEC_UUID);
  // Command characteristic
  BLECharacteristic *command_characteristic = ble_exec_service->createCharacteristic(
                                                                   COMMAND_UUID,
                                                                   BLECharacteristic::PROPERTY_WRITE
                                                                 );
  command_characteristic->setCallbacks(new MyCallbacks());
  // Start services and advertising
  ble_access_service->start();
  ble_color_service->start();  
  ble_device_info_service->start();  
  ble_config_service->start();
  ble_exec_service->start();    
  BLEAdvertising *ble_advertising = ble_server->getAdvertising();
  ble_advertising->start();
  // Set MQTT broker
  client.setServer(configuration.broker_server, 1883);
  client.setCallback(mqttCallback);
  while (!client.connected()) {
    led.blink(1,500,500,255,0,255);
    Serial.println("Connecting to MQTT...");
    if (client.connect(configuration.ble_name, configuration.broker_user, configuration.broker_pass)) {
      Serial.println("connected");  
      led.blink(1,200,200,255,0,255);
      led.blink(1,200,200,0,255,0);
      led.blink(1,200,200,255,0,255);
      led.blink(1,200,200,0,255,0);
      char hello_topic[50] = "";
      strcat(hello_topic,configuration.broker_topic);
      strcat(hello_topic,"/hello"); 
      client.publish(hello_topic, configuration.ble_name);
      // Setup strings
      strcat(alive_topic,configuration.broker_topic);
      strcat(alive_topic,"/alive");
      strcat(color_topic,configuration.broker_topic);
      strcat(color_topic,"/color");
      // Subscribe
      client.subscribe(color_topic);
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      led.blink(1,200,200,255,0,255);
      led.blink(1,200,200,255,0,0);
      led.blink(1,200,200,255,0,255);
      led.blink(1,200,200,255,0,0);
      delay(2000);
    }
  }
  led.blink(2,1000,100,0,255,0);
}

void loop() {

  // Send hertbeat
  if (millis() - last_alive > 5000) {
    client.publish(alive_topic, configuration.color);
    last_alive = millis();
  }
  client.loop();
  number = (int) strtol( &real_time_color[1], NULL, 16);
  led.on(number >> 16, number >> 8 & 0xFF, number & 0xFF);
  delay(100);
}
