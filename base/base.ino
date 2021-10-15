#include <iostream>
#include <string>
using namespace std;
#include <WiFi.h>
#include <ESP32httpUpdate.h>
#include <EEPROM.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <Adafruit_NeoPixel.h>
#include <PubSubClient.h>
#include <ESP32httpUpdate.h>
#include "definitions.h"
#include "led.h"

configuration_struct configuration;
Adafruit_NeoPixel pixels(NUMBER_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
Led led(&pixels);
WiFiClient espClient;
PubSubClient client(espClient);

BLECharacteristic *color_characteristic;
BLECharacteristic *status_characteristic;

char real_time_color[8] = "#0000ff";
int number = 0;
long int last_alive_ts = 0;
long int last_color_ts = 0;
long int last_master_ts = 0;
char alive_topic[50];
char color_topic[50];
bool master = false;
bool slave = false;
char ledlink_status[20];

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {

        configuration_struct configuration;

        EEPROM.begin(eeprom_size);
        EEPROM.get(0, configuration);
        if (pCharacteristic->getUUID().equals(BLEUUID(SSID_UUID))) {
          strcpy(configuration.ssid, value.c_str());
        } else if (pCharacteristic->getUUID().equals(BLEUUID(PASSWORD_UUID))) {
          strcpy(configuration.password, value.c_str());
        } else if (pCharacteristic->getUUID().equals(BLEUUID(OTA_SERVER_UUID))) {
          strcpy(configuration.ota_server, value.c_str());
        } else if (pCharacteristic->getUUID().equals(BLEUUID(BROKER_SERVER_UUID))) {
          strcpy(configuration.broker_server, value.c_str());
        } else if (pCharacteristic->getUUID().equals(BLEUUID(TOPIC_UUID))) {
          strcpy(configuration.broker_topic, value.c_str());
        } else if (pCharacteristic->getUUID().equals(BLEUUID(BROKER_USER_UUID))) {
          strcpy(configuration.broker_user, value.c_str());
        } else if (pCharacteristic->getUUID().equals(BLEUUID(BROKER_PASS_UUID))) {
          strcpy(configuration.broker_pass, value.c_str());
        } else if (pCharacteristic->getUUID().equals(BLEUUID(COLOR_UUID))) {
          strcpy(configuration.color, value.c_str());
        } else if (pCharacteristic->getUUID().equals(BLEUUID(COMMAND_UUID))) {
          if (strcmp(value.c_str(), "reboot") == 0) {
            Serial.print("Reboot");
            ESP.restart();
          } else if (strcmp(value.c_str(), "update") == 0) {
            Serial.print("Update");
            configuration.updated = false;
            EEPROM.put(0, configuration);
            EEPROM.commit();
            ESP.restart();
          }
        }

        EEPROM.put(0, configuration);
        EEPROM.commit();
        delay(100);

      }
    }
};

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, color_topic) == 0) {
    slave = true;
    for (int i = 0; i < length; i++) {
      real_time_color[i] == (char)payload[i];
    }
  }
}

void updateFirmware(char* update_server) {

  char binURL[150] = "";
  strcat(binURL, update_server);
  strcat(binURL, "/firmware/base.bin");
  Serial.println(binURL);

  t_httpUpdate_return ret = ESPhttpUpdate.update( binURL );
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\r\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;
  }
}

void bleSetup() {
  char ble_name[150] = "";
  strcat(ble_name, configuration.ble_name);
  strcat(ble_name, " ");
  strcat(ble_name, configuration.color);
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
  // Real time color characteristic
  color_characteristic = ble_color_service->createCharacteristic(
        REAL_TIME_COLOR_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
      );
  color_characteristic->addDescriptor(new BLE2902());
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
  BLEService *ble_config_service = ble_server->createService(CONFIGURATION_SERVICE_UUID);
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
  // Command service
  BLEService *ble_command_service = ble_server->createService(COMMAND_SERVICE_UUID);
  // Command characteristic
  BLECharacteristic *command_characteristic = ble_command_service->createCharacteristic(
        COMMAND_UUID,
        BLECharacteristic::PROPERTY_WRITE
      );
  command_characteristic->setCallbacks(new MyCallbacks());
  // Status service
  BLEService *ble_status_service = ble_server->createService(STATUS_SERVICE_UUID);
  // Status characteristic
  status_characteristic = ble_status_service->createCharacteristic(
        STATUS_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY
      );
  status_characteristic->addDescriptor(new BLE2902());
  // Start services and advertising
  ble_access_service->start();
  ble_color_service->start();
  ble_device_info_service->start();
  ble_config_service->start();
  ble_command_service->start();
  ble_status_service->start();
  BLEAdvertising *ble_advertising = ble_server->getAdvertising();
  ble_advertising->start();
}

void changeStatus(char state[20]) {
  strcpy(ledlink_status, state);
  status_characteristic->setValue(state);
  status_characteristic->notify();
}

void setup() {
  Serial.begin(baudrate);

  // Read the eeprom
  Serial.println("\r\n\r\nWELCOME 2 LED LINK");
  Serial.println("\r\n");
  led.blink(1, 1000, 1000, 0, 255, 0);
  EEPROM.begin(eeprom_size);
  EEPROM.get(0, configuration);

  // Set up BLE
  bleSetup();
  changeStatus("Booting");

  // Connect to WiFi
  Serial.print("Connecting to ");
  Serial.println(configuration.ssid);
  WiFi.begin(configuration.ssid, configuration.password);
  int start_ts = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start_ts < 5000) {
    led.blink(1, 500, 500, 0, 0, 255);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\r\nConnected");
    changeStatus("WiFi - preparado");

    // Download and update firmware
    if (!configuration.updated) {
      Serial.print("Updating firmware");
      changeStatus("Updating");
      configuration.updated = true;
      EEPROM.put(0, configuration);
      EEPROM.commit();
      updateFirmware(&configuration.ota_server[0]);
    }

    // Set MQTT broker
    Serial.println("Connecting to MQTT");
    client.setServer(configuration.broker_server, 1883);
    client.setCallback(mqttCallback);
    start_ts = millis();
    while (!client.connect(configuration.ble_name, configuration.broker_user, configuration.broker_pass) && millis() - start_ts < 5000) {
      led.blink(1, 500, 500, 255, 0, 255);
    }
    if (client.connected()) {
      Serial.println("\r\nConnected");
      changeStatus("MQTT - preparado");
      // Setup strings
      strcat(alive_topic, configuration.broker_topic);
      strcat(alive_topic, "/alive");
      strcat(color_topic, configuration.broker_topic);
      strcat(color_topic, "/color");
      // Subscribe
      client.subscribe(color_topic);
    } else {
      Serial.print("\r\nFail");
      changeStatus("MQTT - fallo");
    }
    led.blink(2, 1000, 100, 0, 255, 0);
  } else {
    Serial.println("\r\nFail");
      changeStatus("Wifi - fallo");
  }

  // Initialize the time stamps variables
  last_color_ts = millis();
  last_alive_ts = millis();
  last_master_ts = millis();
  client.publish(alive_topic, "configuration.color");

  changeStatus("Conectado");
}

void loop() {

  if (WiFi.status() == WL_CONNECTED) {

    // Become a master
    if (!slave && !master && millis() - last_color_ts > 2000) {
      master = true;
      client.publish(color_topic, real_time_color);
      last_color_ts = millis();
    }

    // Send color as master
    if (master && millis() - last_master_ts > 5000) {
      client.publish(color_topic, real_time_color);
      color_characteristic->setValue(real_time_color);
      color_characteristic->notify();
      last_master_ts = millis();
    }

    // Send hertbeat
    if (millis() - last_alive_ts > 5000) {
      client.publish(alive_topic, configuration.color);
      last_alive_ts = millis();
    }
    client.loop();
    number = (int) strtol( &real_time_color[1], NULL, 16);
    led.on(number >> 16, number >> 8 & 0xFF, number & 0xFF);
    delay(100);
  }
}
