#include <Arduino.h>
#include "definitions.h"
#define DEFINITIONS

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();     

      if (value.length() > 0) {

        configuration_struct configuration;
        
        EEPROM.begin(eeprom_size);  
        EEPROM.get(0, configuration);
        if (pCharacteristic->getUUID().equals(BLEUUID(SSID_UUID))) {
          strcpy(configuration.ssid, value.c_str());
          Serial.print("SSID ");
          Serial.println(configuration.ssid);
        } else if (pCharacteristic->getUUID().equals(BLEUUID(PASSWORD_UUID))) {
          strcpy(configuration.password, value.c_str());
          Serial.print("PASSWORD ");
          Serial.println(configuration.password);
        } else if (pCharacteristic->getUUID().equals(BLEUUID(OTA_SERVER_UUID))) {
          strcpy(configuration.ota_server, value.c_str());
          Serial.print("OTA SERVER ");
          Serial.println(configuration.ota_server);
        } else if (pCharacteristic->getUUID().equals(BLEUUID(BROKER_SERVER_UUID))) {
          strcpy(configuration.broker_server, value.c_str());
          Serial.print("BROKER ");
          Serial.println(configuration.broker_server);
        } else if (pCharacteristic->getUUID().equals(BLEUUID(TOPIC_UUID))) {
          strcpy(configuration.topic, value.c_str());
          Serial.print("TOPIC ");
          Serial.println(configuration.topic);
        } else if (pCharacteristic->getUUID().equals(BLEUUID(COLOR_UUID))) {
          strcpy(configuration.color, value.c_str());
          Serial.print("COLOR ");
          Serial.println(configuration.color);
        } else if (pCharacteristic->getUUID().equals(BLEUUID(COMMAND_UUID))) {
          if(strcmp(value.c_str(), "reboot") == 0){
            Serial.print("Reboot");
            ESP.restart();
          } else if(strcmp(value.c_str(), "update") == 0){
            Serial.print("Update");
            configuration.updated = false;
            EEPROM.put(0,configuration);
            EEPROM.commit();
            ESP.restart();
          }
        }

        EEPROM.put(0,configuration);
        EEPROM.commit();
        delay(100);
        //ESP.restart();
  
      }
    }
};
