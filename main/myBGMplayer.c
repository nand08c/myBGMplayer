#include "esp_log.h"
#include "sdcard.h"

void app_main(void) {
  ESP_LOGI("MY_BGM_PLAYER", "Hello World");
  sdcard_init();
}
