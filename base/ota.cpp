#include <ESP32httpUpdate.h>

void updateFirmware(char* update_server) {

  char binURL[150] = "";
  strcat(binURL,update_server);
  strcat(binURL,"/firmware/base.bin");
  Serial.println(binURL);

  t_httpUpdate_return ret = ESPhttpUpdate.update( binURL );
  switch(ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\r\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;
  }
}
