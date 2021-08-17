char version_code[2] = "1";
int baudrate = 115200;
int eeprom_size = 512;
#define COLOR_SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define REAL_TIME_COLOR_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define COLOR_UUID "737bfbc4-3d44-44a0-bad0-3314a5714180"
#define CONFIGURATION_UUID        "e63589ad-5603-49c8-b82d-b608c65d8d9c"
#define SSID_UUID "36fb19d4-f1e4-4892-b35b-68e364773f7b"
#define PASSWORD_UUID "caaaa902-4259-4e34-9c92-f24105608a53"
#define OTA_SERVER_UUID "d6c28322-259e-43c0-9db2-4c12f60de876"
#define BROKER_SERVER_UUID "02e9cf6d-5f8c-41b6-bdbe-089178521b63"
#define TOPIC_UUID "3eee7dd7-94ce-4206-a163-dc8cb75d2751"
#define BROKER_USER_UUID "46e9b927-dd23-4fac-ba8f-516af8a9837c"
#define BROKER_PASS_UUID "3f85b4e2-fe1f-4c17-a7d3-8e1de0fd5ebd"
#define EXEC_UUID        "d9b655f5-0c49-491e-a4a0-c836826b30cf"
#define COMMAND_UUID        "5ade4357-0226-4e39-b95d-b129f3f45be1"

struct configuration_struct {
  char ssid[50];
  char password[50];
  char ota_server[100];
  bool updated;
  char broker_server[100];
  char broker_topic[50];
  char broker_user[25];
  char broker_pass[25];
  char ble_name[50];
  char color[8];
};
